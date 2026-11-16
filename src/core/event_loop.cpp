// src/core/event_loop.cpp
#include <pingdart/core/event_loop.h>
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace pingdart::core {

namespace {

constexpr size_t kMaxMicrotaskBatch = 1024;

struct DelayedTask {
    TimePoint deadline;
    uint64_t sequence;
    Task task;

    bool operator>(const DelayedTask& other) const noexcept {
        if (deadline != other.deadline) {
            return deadline > other.deadline;
        }
        return sequence > other.sequence;
    }
};

class EventLoopImpl final : public EventLoop {
public:
    EventLoopImpl() : thread_id_(std::this_thread::get_id()) {}
    ~EventLoopImpl() override {
        terminate(-1);
    }

    void set_wakeup_handle(IWakeupHandle* handle) noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        wakeup_handle_ = handle;
    }

    [[nodiscard]] IWakeupHandle* wakeup_handle() const noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        return wakeup_handle_;
    }

    [[nodiscard]] std::optional<Milliseconds> next_deadline() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (delayed_tasks_.empty()) {
            return std::nullopt;
        }
        const auto now = Clock::now();
        const auto earliest = delayed_tasks_.top().deadline;
        if (earliest <= now) {
            return Milliseconds::zero();
        }
        return std::chrono::duration_cast<Milliseconds>(earliest - now);
    }

    [[nodiscard]] bool has_ready_tasks() const noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (terminated_) return false;
        if (!microtasks_.empty() || !tasks_.empty()) return true;
        if (!delayed_tasks_.empty() && delayed_tasks_.top().deadline <= Clock::now()) return true;
        return false;
    }

    void post_task(Task task) override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push_back(std::move(task));
            notify_wakeup_locked();
        }
        cv_.notify_one();
    }

    void post_delayed_task(Task task, Milliseconds delay) override {
        const auto deadline = Clock::now() + delay;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            delayed_tasks_.push(DelayedTask{
                deadline,
                ++sequence_counter_,
                std::move(task)
            });
            notify_wakeup_locked();
        }
        cv_.notify_one();
    }

    void post_microtask(Task task) override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            microtasks_.push_back(std::move(task));
            notify_wakeup_locked();
        }
        cv_.notify_one();
    }

    bool run_one(Milliseconds timeout) override {
        std::unique_lock<std::mutex> lock(mutex_);

        if (terminated_) {
            return false;
        }

        promote_delayed_tasks_locked();
        drain_microtasks_locked(lock);

        if (!tasks_.empty()) {
            Task task = std::move(tasks_.front());
            tasks_.pop_front();
            lock.unlock();
            task();
            return true;
        }

        if (timeout == Milliseconds::zero()) {
            return false;
        }

        const auto now = Clock::now();
        TimePoint wait_deadline = now + timeout;
        if (!delayed_tasks_.empty()) {
            wait_deadline = std::min(wait_deadline, delayed_tasks_.top().deadline);
        }

        cv_.wait_until(lock, wait_deadline, [this]() {
            return terminated_ || !tasks_.empty() || !microtasks_.empty() ||
                   (!delayed_tasks_.empty() && delayed_tasks_.top().deadline <= Clock::now());
        });

        if (terminated_) {
            return false;
        }

        promote_delayed_tasks_locked();
        drain_microtasks_locked(lock);

        if (!tasks_.empty()) {
            Task task = std::move(tasks_.front());
            tasks_.pop_front();
            lock.unlock();
            task();
            return true;
        }

        return false;
    }

    int run() override {
        thread_id_ = std::this_thread::get_id();
        is_running_ = true;

        std::unique_lock<std::mutex> lock(mutex_);
        while (!terminated_) {
            promote_delayed_tasks_locked();
            drain_microtasks_locked(lock);

            if (!tasks_.empty()) {
                Task task = std::move(tasks_.front());
                tasks_.pop_front();
                lock.unlock();
                task();
                lock.lock();
                continue;
            }

            if (terminated_) {
                break;
            }

            if (!delayed_tasks_.empty()) {
                const auto deadline = delayed_tasks_.top().deadline;
                cv_.wait_until(lock, deadline, [this]() {
                    return terminated_ || !tasks_.empty() || !microtasks_.empty() ||
                           (!delayed_tasks_.empty() && delayed_tasks_.top().deadline <= Clock::now());
                });
            } else {
                cv_.wait(lock, [this]() {
                    return terminated_ || !tasks_.empty() || !microtasks_.empty() ||
                           !delayed_tasks_.empty();
                });
            }
        }

        is_running_ = false;
        return exit_code_;
    }

    void terminate(int exit_code) override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!terminated_) {
                terminated_ = true;
                exit_code_ = exit_code;
                notify_wakeup_locked();
            }
        }
        cv_.notify_all();
    }

    [[nodiscard]] bool is_on_loop_thread() const noexcept override {
        return std::this_thread::get_id() == thread_id_;
    }

    [[nodiscard]] bool is_running() const noexcept override {
        return is_running_;
    }

private:
    void notify_wakeup_locked() noexcept {
        if (wakeup_handle_) {
            wakeup_handle_->wake();
        }
    }

    void promote_delayed_tasks_locked() {
        const auto now = Clock::now();
        while (!delayed_tasks_.empty() && delayed_tasks_.top().deadline <= now) {
            tasks_.push_back(std::move(const_cast<DelayedTask&>(delayed_tasks_.top()).task));
            delayed_tasks_.pop();
        }
    }

    void drain_microtasks_locked(std::unique_lock<std::mutex>& lock) {
        size_t count = 0;
        while (!microtasks_.empty() && count < kMaxMicrotaskBatch) {
            Task task = std::move(microtasks_.front());
            microtasks_.pop_front();
            lock.unlock();
            task();
            lock.lock();
            ++count;
        }
    }

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<Task> tasks_;
    std::deque<Task> microtasks_;
    std::priority_queue<DelayedTask, std::vector<DelayedTask>, std::greater<DelayedTask>> delayed_tasks_;
    uint64_t sequence_counter_{0};
    std::thread::id thread_id_;
    IWakeupHandle* wakeup_handle_{nullptr};
    bool is_running_{false};
    bool terminated_{false};
    int exit_code_{0};
};

} // namespace

std::unique_ptr<EventLoop> EventLoop::create() {
    return std::make_unique<EventLoopImpl>();
}

} // namespace pingdart::core

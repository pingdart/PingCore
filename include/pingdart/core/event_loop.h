// include/pingdart/core/event_loop.h
#pragma once
#include <chrono>
#include <functional>
#include <memory>
#include <optional>

namespace pingdart::core {

using Task = std::function<void()>;
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Milliseconds = std::chrono::milliseconds;

/// Abstract interface for platform-native wakeup primitives (e.g., Win32 SetEvent, Linux eventfd write)
class IWakeupHandle {
public:
    virtual ~IWakeupHandle() = default;
    virtual void wake() = 0;
};

class EventLoop {
public:
    virtual ~EventLoop() = default;

    // Dispatches a task to execute on the next turn of the loop
    virtual void post_task(Task task) = 0;

    // Schedules a task to execute after a specified delay
    virtual void post_delayed_task(Task task, Milliseconds delay) = 0;

    // Schedules high-priority microtask executed before next macrotask
    virtual void post_microtask(Task task) = 0;

    // Runs a single turn of the event loop with an optional maximum wait timeout
    virtual bool run_one(Milliseconds timeout = Milliseconds::zero()) = 0;

    // Runs the loop until terminate() is called
    virtual int run() = 0;

    // Signals the event loop to stop processing and exit run()
    virtual void terminate(int exit_code = 0) = 0;

    // Returns whether current execution thread is the loop's owning UI thread
    [[nodiscard]] virtual bool is_on_loop_thread() const noexcept = 0;

    // Returns whether the loop is currently running
    [[nodiscard]] virtual bool is_running() const noexcept = 0;

    // Registers a platform wakeup handle
    virtual void set_wakeup_handle(IWakeupHandle* handle) noexcept = 0;
    [[nodiscard]] virtual IWakeupHandle* wakeup_handle() const noexcept = 0;

    // Returns the remaining time until the next scheduled delayed task, or nullopt if none
    [[nodiscard]] virtual std::optional<Milliseconds> next_deadline() const = 0;

    // Returns whether any microtasks, macrotasks, or expired delayed tasks are ready to run
    [[nodiscard]] virtual bool has_ready_tasks() const noexcept = 0;

    // Creates the platform-appropriate event loop instance
    static std::unique_ptr<EventLoop> create();
};

} // namespace pingdart::core

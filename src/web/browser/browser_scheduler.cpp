#include "browser_scheduler.h"

namespace pingdart::web {

void BrowserScheduler::request_animation_frame(FrameCallback callback) {
    if (callback) {
        frame_callbacks_.push_back(std::move(callback));
    }
}

void BrowserScheduler::queue_microtask(MicrotaskCallback callback) {
    if (callback) {
        microtasks_.push(std::move(callback));
    }
}

void BrowserScheduler::set_timeout(uint32_t delay_ms, MicrotaskCallback callback) {
    if (callback) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
        delayed_tasks_.push_back(DelayedTask{deadline, std::move(callback)});
    }
}

bool BrowserScheduler::has_pending_tasks() const noexcept {
    return !frame_callbacks_.empty() || !microtasks_.empty() || !delayed_tasks_.empty();
}

size_t BrowserScheduler::drain_bounded_microtasks(bool* out_yield_required) {
    size_t executed = 0;

    while (!microtasks_.empty() && executed < kMaxMicrotasksPerTurn) {
        auto task = std::move(microtasks_.front());
        microtasks_.pop();
        if (task) {
            task();
            executed++;
        }
    }

    const bool yield_required = !microtasks_.empty();
    if (yield_required) {
        yield_count_++;
    }
    if (out_yield_required) {
        *out_yield_required = yield_required;
    }

    return executed;
}

size_t BrowserScheduler::drain_microtasks() {
    return drain_bounded_microtasks(nullptr);
}

bool BrowserScheduler::step_frame(double timestamp_ms) {
    // 1. Drain delayed timers that reached deadline
    auto now = std::chrono::steady_clock::now();
    for (auto it = delayed_tasks_.begin(); it != delayed_tasks_.end(); ) {
        if (it->deadline <= now) {
            if (it->callback) {
                it->callback();
            }
            it = delayed_tasks_.erase(it);
        } else {
            ++it;
        }
    }

    // 2. Drain microtasks prior to frame callbacks with starvation limit
    drain_bounded_microtasks(nullptr);

    if (frame_callbacks_.empty()) {
        return false;
    }

    std::vector<FrameCallback> active_callbacks;
    active_callbacks.swap(frame_callbacks_);

    for (const auto& cb : active_callbacks) {
        if (cb) {
            cb(timestamp_ms);
        }
    }

    // 3. Drain post-frame microtasks
    drain_bounded_microtasks(nullptr);

    return true;
}

void BrowserScheduler::reset() {
    frame_callbacks_.clear();
    while (!microtasks_.empty()) {
        microtasks_.pop();
    }
    delayed_tasks_.clear();
    yield_count_ = 0;
}

} // namespace pingdart::web

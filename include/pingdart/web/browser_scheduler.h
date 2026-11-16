#pragma once

#include <functional>
#include <cstdint>

namespace pingdart::web {

using FrameCallback = std::function<void(double timestamp_ms)>;
using MicrotaskCallback = std::function<void()>;

class IBrowserScheduler {
public:
    virtual ~IBrowserScheduler() = default;

    // Schedules a callback on the browser's requestAnimationFrame cycle
    virtual void request_animation_frame(FrameCallback callback) = 0;

    // Schedules a microtask (runs after the current task before browser repaints)
    virtual void queue_microtask(MicrotaskCallback callback) = 0;

    // Schedules a delayed macro-timer
    virtual void set_timeout(uint32_t delay_ms, MicrotaskCallback callback) = 0;

    // Query whether any frame callbacks or microtasks are queued
    [[nodiscard]] virtual bool has_pending_tasks() const noexcept = 0;
};

} // namespace pingdart::web

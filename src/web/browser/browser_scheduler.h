#pragma once

#include <pingdart/web/browser_scheduler.h>
#include <vector>
#include <queue>
#include <chrono>

namespace pingdart::web {

class BrowserScheduler final : public IBrowserScheduler {
public:
    static constexpr size_t kMaxMicrotasksPerTurn = 32;

    BrowserScheduler() = default;
    ~BrowserScheduler() override = default;

    void request_animation_frame(FrameCallback callback) override;
    void queue_microtask(MicrotaskCallback callback) override;
    void set_timeout(uint32_t delay_ms, MicrotaskCallback callback) override;

    [[nodiscard]] bool has_pending_tasks() const noexcept override;

    // Headless / Test / Turn execution helpers
    // Executes up to kMaxMicrotasksPerTurn to prevent browser rendering starvation.
    // Returns number of executed microtasks. If unexecuted tasks remain, yield_required is true.
    size_t drain_bounded_microtasks(bool* out_yield_required = nullptr);
    size_t drain_microtasks();
    bool step_frame(double timestamp_ms = 0.0);
    void reset();

    [[nodiscard]] size_t pending_frame_count() const noexcept { return frame_callbacks_.size(); }
    [[nodiscard]] size_t pending_microtask_count() const noexcept { return microtasks_.size(); }
    [[nodiscard]] uint32_t yield_count() const noexcept { return yield_count_; }

private:
    std::vector<FrameCallback> frame_callbacks_;
    std::queue<MicrotaskCallback> microtasks_;

    struct DelayedTask {
        std::chrono::steady_clock::time_point deadline;
        MicrotaskCallback callback;
    };
    std::vector<DelayedTask> delayed_tasks_;
    uint32_t yield_count_{0};
};

} // namespace pingdart::web

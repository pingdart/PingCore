#pragma once

#include "display_list.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>

namespace pingdart::rhi {

enum class SlotState : uint8_t {
    Empty = 0,
    Writing,
    Ready,
    Rendering,
    GpuInFlight,
    Retired
};

struct MailboxSlot {
    core::RefPtr<DisplayList> display_list;
    uint64_t sequence_number{0};
    uint64_t gpu_generation{0};
    std::atomic<SlotState> state{SlotState::Empty};
};

class DisplayListMailbox {
public:
    DisplayListMailbox();
    ~DisplayListMailbox() = default;

    DisplayListMailbox(const DisplayListMailbox&) = delete;
    DisplayListMailbox& operator=(const DisplayListMailbox&) = delete;

    // Called on UI Thread: Publishes a new DisplayList without blocking
    uint64_t publish(core::RefPtr<DisplayList> display_list);

    // Called on Render Thread: Acquires the latest published DisplayList
    [[nodiscard]] core::RefPtr<DisplayList> acquire_latest(uint64_t* out_seq = nullptr);

    // Called on Render Thread after GPU command submission: transitions slot to GpuInFlight
    void mark_rendering_submitted(uint64_t gpu_fence_generation);

    // Called on Render Thread when GPU fence signals completion: safely retires slot
    void notify_gpu_complete(uint64_t completed_gpu_generation);

    // Release rendering slot synchronously (e.g. mock/test mode or CPU wait_idle)
    void release_rendering();

    [[nodiscard]] uint64_t latest_published_sequence() const noexcept {
        return next_sequence_.load(std::memory_order_relaxed) - 1;
    }

private:
    static constexpr size_t NUM_SLOTS = 3;
    std::array<MailboxSlot, NUM_SLOTS> slots_;
    std::atomic<uint64_t> next_sequence_{1};
    std::atomic<uint64_t> completed_gpu_generation_{0};
    int active_rendering_slot_{-1};
    std::mutex publish_mutex_;
};

} // namespace pingdart::rhi

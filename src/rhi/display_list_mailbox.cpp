#include <pingdart/rhi/display_list_mailbox.h>

namespace pingdart::rhi {

DisplayListMailbox::DisplayListMailbox() = default;

uint64_t DisplayListMailbox::publish(core::RefPtr<DisplayList> display_list) {
    std::lock_guard<std::mutex> lock(publish_mutex_);

    uint64_t completed_gen = completed_gpu_generation_.load(std::memory_order_acquire);

    // Find a slot that is Empty, or Retired AND has finished GPU execution
    int candidate_idx = -1;
    for (size_t i = 0; i < NUM_SLOTS; ++i) {
        SlotState st = slots_[i].state.load(std::memory_order_acquire);
        if (st == SlotState::Empty) {
            candidate_idx = static_cast<int>(i);
            break;
        }
        if (st == SlotState::Retired && slots_[i].gpu_generation <= completed_gen) {
            candidate_idx = static_cast<int>(i);
            break;
        }
    }

    // If none are completely retired, find the oldest Ready slot that hasn't been rendered
    if (candidate_idx == -1) {
        uint64_t oldest_seq = UINT64_MAX;
        for (size_t i = 0; i < NUM_SLOTS; ++i) {
            SlotState st = slots_[i].state.load(std::memory_order_acquire);
            if (st == SlotState::Ready && slots_[i].sequence_number < oldest_seq) {
                oldest_seq = slots_[i].sequence_number;
                candidate_idx = static_cast<int>(i);
            }
        }
    }

    // Fallback: pick any slot not currently in Rendering or GpuInFlight
    if (candidate_idx == -1) {
        for (size_t i = 0; i < NUM_SLOTS; ++i) {
            SlotState st = slots_[i].state.load(std::memory_order_acquire);
            if (st != SlotState::Rendering && st != SlotState::GpuInFlight) {
                candidate_idx = static_cast<int>(i);
                break;
            }
        }
    }

    if (candidate_idx == -1) {
        // All slots busy in Rendering or GPU execution; fallback to slot 0
        candidate_idx = 0;
    }

    auto& slot = slots_[static_cast<size_t>(candidate_idx)];
    slot.state.store(SlotState::Writing, std::memory_order_release);
    slot.display_list = std::move(display_list);
    uint64_t seq = next_sequence_.fetch_add(1, std::memory_order_relaxed);
    slot.sequence_number = seq;
    slot.state.store(SlotState::Ready, std::memory_order_release);

    return seq;
}

core::RefPtr<DisplayList> DisplayListMailbox::acquire_latest(uint64_t* out_seq) {
    int best_idx = -1;
    uint64_t best_seq = 0;

    for (size_t i = 0; i < NUM_SLOTS; ++i) {
        if (slots_[i].state.load(std::memory_order_acquire) == SlotState::Ready) {
            if (slots_[i].sequence_number > best_seq) {
                best_seq = slots_[i].sequence_number;
                best_idx = static_cast<int>(i);
            }
        }
    }

    if (best_idx == -1) {
        return nullptr;
    }

    // Retire any older unconsumed Ready slots (stale frame dropping)
    for (size_t i = 0; i < NUM_SLOTS; ++i) {
        if (static_cast<int>(i) != best_idx &&
            slots_[i].state.load(std::memory_order_acquire) == SlotState::Ready) {
            slots_[i].display_list.reset();
            slots_[i].state.store(SlotState::Retired, std::memory_order_release);
        }
    }

    // Release previously active rendering slot if not already submitted
    release_rendering();

    auto& chosen = slots_[static_cast<size_t>(best_idx)];
    chosen.state.store(SlotState::Rendering, std::memory_order_release);
    active_rendering_slot_ = best_idx;

    if (out_seq) {
        *out_seq = chosen.sequence_number;
    }

    return chosen.display_list;
}

void DisplayListMailbox::mark_rendering_submitted(uint64_t gpu_fence_generation) {
    if (active_rendering_slot_ >= 0) {
        auto& slot = slots_[static_cast<size_t>(active_rendering_slot_)];
        slot.gpu_generation = gpu_fence_generation;
        slot.state.store(SlotState::GpuInFlight, std::memory_order_release);
        active_rendering_slot_ = -1;
    }
}

void DisplayListMailbox::notify_gpu_complete(uint64_t completed_gpu_generation) {
    completed_gpu_generation_.store(completed_gpu_generation, std::memory_order_release);
    for (size_t i = 0; i < NUM_SLOTS; ++i) {
        if (slots_[i].state.load(std::memory_order_acquire) == SlotState::GpuInFlight) {
            if (slots_[i].gpu_generation <= completed_gpu_generation) {
                slots_[i].display_list.reset();
                slots_[i].state.store(SlotState::Retired, std::memory_order_release);
            }
        }
    }
}

void DisplayListMailbox::release_rendering() {
    if (active_rendering_slot_ >= 0) {
        auto& slot = slots_[static_cast<size_t>(active_rendering_slot_)];
        slot.display_list.reset();
        slot.state.store(SlotState::Retired, std::memory_order_release);
        active_rendering_slot_ = -1;
    }
}

} // namespace pingdart::rhi

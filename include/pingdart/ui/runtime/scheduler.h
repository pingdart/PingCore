// include/pingdart/ui/runtime/scheduler.h
// PingDartUI UI-Thread Scheduler, Batched Updates & Dirty Tracking (PKG-138)
#pragma once

#include <pingdart/ui/runtime/component_instance.h>
#include <vector>
#include <algorithm>
#include <functional>
#include <mutex>
#include <unordered_set>
#include <atomic>

namespace pingdart::ui {

class Scheduler {
private:
    std::mutex mutex_{};
    std::vector<std::shared_ptr<ComponentInstance>> dirty_queue_{};
    std::unordered_set<ComponentInstance*> dirty_set_{};
    int batch_depth_{0};
    std::atomic<size_t> total_flushes_{0};
    std::atomic<size_t> total_batched_updates_{0};

    std::function<void(std::shared_ptr<ComponentInstance>)> update_handler_{nullptr};

public:
    static Scheduler& instance() {
        static Scheduler s_scheduler;
        return s_scheduler;
    }

    void set_update_handler(std::function<void(std::shared_ptr<ComponentInstance>)> h) {
        std::lock_guard<std::mutex> lock(mutex_);
        update_handler_ = std::move(h);
    }

    void mark_dirty(std::shared_ptr<ComponentInstance> instance) {
        if (!instance) return;
        std::lock_guard<std::mutex> lock(mutex_);
        total_batched_updates_++;
        instance->set_dirty(true);

        if (dirty_set_.find(instance.get()) == dirty_set_.end()) {
            dirty_set_.insert(instance.get());
            dirty_queue_.push_back(instance);
        }

        if (batch_depth_ == 0) {
            flush_internal();
        }
    }

    void batch(const std::function<void()>& fn) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            batch_depth_++;
        }
        try {
            if (fn) fn();
        } catch (...) {
            std::lock_guard<std::mutex> lock(mutex_);
            batch_depth_--;
            throw;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            batch_depth_--;
            if (batch_depth_ == 0) {
                flush_internal();
            }
        }
    }

    void flush() {
        std::lock_guard<std::mutex> lock(mutex_);
        flush_internal();
    }

    size_t flush_count() const noexcept { return total_flushes_.load(); }
    size_t batched_updates_count() const noexcept { return total_batched_updates_.load(); }
    size_t dirty_count() {
        std::lock_guard<std::mutex> lock(mutex_);
        return dirty_queue_.size();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        dirty_queue_.clear();
        dirty_set_.clear();
        batch_depth_ = 0;
        total_flushes_ = 0;
        total_batched_updates_ = 0;
    }

private:
    void flush_internal() {
        if (dirty_queue_.empty()) return;

        // Sort by tree depth ascending (ancestor before descendant)
        std::sort(dirty_queue_.begin(), dirty_queue_.end(), 
            [](const std::shared_ptr<ComponentInstance>& a, const std::shared_ptr<ComponentInstance>& b) {
                return a->depth() < b->depth();
            });

        auto queue_copy = std::move(dirty_queue_);
        dirty_queue_.clear();
        dirty_set_.clear();
        total_flushes_++;

        for (auto& inst : queue_copy) {
            if (inst && inst->is_dirty()) {
                inst->set_dirty(false);
                if (update_handler_) {
                    update_handler_(inst);
                }
                inst->update_lifecycle();
            }
        }
    }
};

} // namespace pingdart::ui

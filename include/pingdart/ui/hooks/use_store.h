// include/pingdart/ui/hooks/use_store.h
// PingDartUI Global Application Store Hook (PKG-138)
#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <cstdint>
#include <stdexcept>

namespace pingdart::ui {

template <typename T>
class Store {
public:
    using Snapshot = std::shared_ptr<const T>;
    using Listener = std::function<void(const T&)>;
    using SubscriptionId = std::uint64_t;

private:
    mutable std::mutex mutex_;

    Snapshot state_;

    SubscriptionId next_id_ = 1;

    std::unordered_map<
        SubscriptionId,
        Listener
    > listeners_;

    std::vector<Listener> copy_listeners() const {
        std::vector<Listener> result;

        result.reserve(listeners_.size());

        for (const auto& [id, listener] : listeners_) {
            result.push_back(listener);
        }

        return result;
    }

    static void notify(
        const Snapshot& snapshot,
        const std::vector<Listener>& listeners
    ) {
        for (const auto& listener : listeners) {
            if (listener) {
                listener(*snapshot);
            }
        }
    }

public:
    explicit Store(T initial)
        : state_(
            std::make_shared<const T>(
                std::move(initial)
            )
        ) {}

    Snapshot snapshot() const {
        std::lock_guard lock(mutex_);
        return state_;
    }

    T get() const {
        return *snapshot();
    }

    void set(T value) {

        auto next = std::make_shared<const T>(
            std::move(value)
        );

        std::vector<Listener> listeners;

        {
            std::lock_guard lock(mutex_);

            listeners = copy_listeners();
            state_ = next;
        }

        notify(next, listeners);
    }

    template <typename F>
    void update(F&& mutator) {

        Snapshot next;
        std::vector<Listener> listeners;

        {
            std::lock_guard lock(mutex_);

            T updated = *state_;

            std::invoke(mutator, updated);

            next = std::make_shared<const T>(
                std::move(updated)
            );

            listeners = copy_listeners();
            state_ = next;
        }

        notify(next, listeners);
    }

    SubscriptionId subscribe(Listener listener) {

        if (!listener) {
            throw std::invalid_argument(
                "Invalid store listener"
            );
        }

        std::lock_guard lock(mutex_);

        auto id = next_id_++;

        listeners_.emplace(
            id, std::move(listener)
        );

        return id;
    }

    void unsubscribe(SubscriptionId id) {

        std::lock_guard lock(mutex_);

        listeners_.erase(id);
    }
};


template <typename T>
std::shared_ptr<Store<T>> createStore(T initial) {

    return std::make_shared<Store<T>>(
        std::move(initial)
    );
}

} // namespace pingdart::ui
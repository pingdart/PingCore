// include/pingdart/ui/hooks/use_context.h
// PingDartUI React-like useContext Hook & Context Provider (PKG-138)
#pragma once

#include <memory>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <vector>
#include <utility>
#include <cstdint>

namespace pingdart::ui {

template <typename T>
class Context {
public:
    using Callback = std::function<void(const T&)>;

private:
    struct Storage {
        explicit Storage(T initial)
            : value(std::move(initial)) {}

        T value;
        std::mutex mutex;

        std::uint64_t next_id = 1;

        std::unordered_map<
            std::uint64_t,
            Callback
        > subscribers;
    };

    std::shared_ptr<Storage> storage_;

public:
    explicit Context(T initial)
        : storage_(
            std::make_shared<Storage>(
                std::move(initial)
            )
        ) {}

    T consume() const {
        std::lock_guard lock(storage_->mutex);
        return storage_->value;
    }

    void provide(T value) const {

        std::vector<Callback> callbacks;

        {
            std::lock_guard lock(storage_->mutex);

            storage_->value = value;

            for (auto& [id, callback] :
                 storage_->subscribers) {
                callbacks.push_back(callback);
            }
        }

        // Notify outside the lock.
        for (auto& callback : callbacks) {
            callback(value);
        }
    }

    std::uint64_t subscribe(Callback callback) const {
        std::lock_guard lock(storage_->mutex);

        auto id = storage_->next_id++;

        storage_->subscribers.emplace(
            id, std::move(callback)
        );

        return id;
    }

    void unsubscribe(std::uint64_t id) const {
        std::lock_guard lock(storage_->mutex);

        storage_->subscribers.erase(id);
    }
};

template <typename T>
Context<T> createContext(T value) {
    return Context<T>(std::move(value));
}

template <typename T>
T useContext(const Context<T>& context) {
    return context.consume();
}

} // namespace pingdart::ui
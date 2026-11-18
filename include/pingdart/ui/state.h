// include/pingdart/ui/state.h
// PingDartUI Persistent Reactive State Registry (PKG-136)
#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>
#include <any>
#include <mutex>

namespace pingdart::ui {

// Thread-safe Persistent State Registry
class StateRegistry {
public:
    static StateRegistry& instance() {
        static StateRegistry s_reg;
        return s_reg;
    }

    template <typename T>
    std::shared_ptr<T> get_or_create(const std::string& key, T initial_val) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = store_.find(key);
        if (it != store_.end()) {
            try {
                return std::any_cast<std::shared_ptr<T>>(it->second);
            } catch (...) {}
        }
        auto ptr = std::make_shared<T>(std::move(initial_val));
        store_[key] = ptr;
        return ptr;
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        store_.clear();
    }

private:
    std::mutex mutex_{};
    std::unordered_map<std::string, std::any> store_{};
};

template <typename T>
class State {
private:
    struct Storage {
        std::shared_ptr<T> val_ptr;
        std::vector<std::function<void(const T&)>> listeners;

        explicit Storage(std::shared_ptr<T> p) : val_ptr(std::move(p)) {}
    };

    std::shared_ptr<Storage> storage_;

public:
    explicit State(T initial_val)
        : storage_(std::make_shared<Storage>(std::make_shared<T>(std::move(initial_val)))) {}

    State(const std::string& persistent_key, T initial_val) {
        auto persistent_ptr = StateRegistry::instance().get_or_create<T>(persistent_key, std::move(initial_val));
        storage_ = std::make_shared<Storage>(persistent_ptr);
    }

    const T& get() const {
        return *storage_->val_ptr;
    }

    void set(T new_val) {
        *storage_->val_ptr = std::move(new_val);
        for (const auto& listener : storage_->listeners) {
            if (listener) listener(*storage_->val_ptr);
        }
    }

    void subscribe(std::function<void(const T&)> listener) {
        storage_->listeners.push_back(std::move(listener));
    }

    operator const T&() const {
        return get();
    }
};

template <typename T>
inline State<T> useState(T initial_value) {
    return State<T>(std::move(initial_value));
}

template <typename T>
inline State<T> useState(const std::string& key, T initial_value) {
    return State<T>(key, std::move(initial_value));
}

template <typename T>
inline State<T> useSignal(T initial_value) {
    return State<T>(std::move(initial_value));
}

template <typename T>
inline State<T> useSignal(const std::string& key, T initial_value) {
    return State<T>(key, std::move(initial_value));
}

} // namespace pingdart::ui

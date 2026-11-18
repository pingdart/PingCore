// include/pingdart/ui/state.h
// PingDartUI Reactive State Management (PKG-135)
#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace pingdart::ui {

template <typename T>
class State {
private:
    struct Storage {
        T value;
        std::vector<std::function<void(const T&)>> listeners;

        explicit Storage(T val) : value(std::move(val)) {}
    };

    std::shared_ptr<Storage> storage_;

public:
    explicit State(T initial_val)
        : storage_(std::make_shared<Storage>(std::move(initial_val))) {}

    const T& get() const {
        return storage_->value;
    }

    void set(T new_val) {
        storage_->value = std::move(new_val);
        for (const auto& listener : storage_->listeners) {
            if (listener) listener(storage_->value);
        }
    }

    void subscribe(std::function<void(const T&)> listener) {
        storage_->listeners.push_back(std::move(listener));
    }

    // Implicit conversion to const T&
    operator const T&() const {
        return get();
    }
};

template <typename T>
inline State<T> useState(T initial_value) {
    return State<T>(std::move(initial_value));
}

template <typename T>
inline State<T> useSignal(T initial_value) {
    return State<T>(std::move(initial_value));
}

} // namespace pingdart::ui

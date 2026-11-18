// include/pingdart/ui/hooks/use_reducer.h
// PingDartUI React-like useReducer Hook (PKG-138)
#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace pingdart::ui {

template <typename State, typename Action>
class ReducerStore {
public:
    using ReducerFn = std::function<State(const State&, const Action&)>;

private:
    State state_;
    ReducerFn reducer_;
    std::vector<std::function<void(const State&)>> listeners_{};

public:
    ReducerStore(ReducerFn reducer, State initial_state)
        : state_(std::move(initial_state)), reducer_(std::move(reducer)) {}

    const State& state() const noexcept { return state_; }

    void dispatch(const Action& action) {
        state_ = reducer_(state_, action);
        for (const auto& l : listeners_) {
            if (l) l(state_);
        }
    }

    void subscribe(std::function<void(const State&)> listener) {
        listeners_.push_back(std::move(listener));
    }
};

template <typename State, typename Action>
class ReducerHandle {
private:
    std::shared_ptr<ReducerStore<State, Action>> store_;

public:
    explicit ReducerHandle(std::shared_ptr<ReducerStore<State, Action>> store)
        : store_(std::move(store)) {}

    const State& state() const noexcept { return store_->state(); }
    void dispatch(const Action& action) { store_->dispatch(action); }
    void subscribe(std::function<void(const State&)> l) { store_->subscribe(std::move(l)); }
};

template <typename State, typename Action>
inline ReducerHandle<State, Action> useReducer(
    std::function<State(const State&, const Action&)> reducer,
    State initial_state) {
    auto store = std::make_shared<ReducerStore<State, Action>>(std::move(reducer), std::move(initial_state));
    return ReducerHandle<State, Action>(store);
}

} // namespace pingdart::ui

// include/pingdart/ui/hooks/use_ref.h
// PingDartUI React-like useRef Hook (PKG-138)
#pragma once

#include <pingdart/ui/runtime/dependency_tracker.h>
#include <pingdart/ui/runtime/component_instance.h>

#include <memory>
#include <utility>
#include <stdexcept>

namespace pingdart::ui {

template <typename T>
class Ref {
public:
    T current;

    explicit Ref(T initial)
        : current(std::move(initial)) {}
};


template <typename T>
struct RefSlot {
    std::shared_ptr<Ref<T>> reference;
};


template <typename T>
std::shared_ptr<Ref<T>> useRef(T initial) {

    auto inst = DependencyTracker::active_instance();

    if (!inst) {
        throw std::logic_error(
            "useRef requires an active component"
        );
    }

    auto& slot =
        inst->template next_hook<RefSlot<T>>();

    if (!slot.reference) {

        slot.reference =
            std::make_shared<Ref<T>>(
                std::move(initial)
            );
    }

    return slot.reference;
}

} // namespace pingdart::ui
#pragma once

#include <pingdart/ui/runtime/component_instance.h>
#include <pingdart/ui/runtime/dependency_tracker.h>
#include <pingdart/ui/runtime/hook_types.h>

#include <functional>
#include <memory>
#include <utility>
#include <type_traits>
#include <stdexcept>

namespace pingdart::ui {

template <typename F>
struct CallbackSlot {
    std::shared_ptr<F> callback;
    HookDeps dependencies;
};

template <typename F>
auto useCallback(F&& callback, HookDeps deps = {}) {

    using Fn = std::decay_t<F>;
    using Slot = CallbackSlot<Fn>;

    auto inst = DependencyTracker::active_instance();

    if (!inst) {
        throw std::logic_error(
            "useCallback requires an active component"
        );
    }

    auto& slot = inst->template next_hook<Slot>();

    if (!slot.callback || slot.dependencies != deps) {

        slot.callback = std::make_shared<Fn>(
            std::forward<F>(callback)
        );

        slot.dependencies = std::move(deps);
    }

    return slot.callback;
}

} // namespace pingdart::ui
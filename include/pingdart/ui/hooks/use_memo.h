// include/pingdart/ui/hooks/use_memo.h
// PingDartUI React-like useMemo Hook (PKG-138)
#pragma once

#include <pingdart/ui/runtime/dependency_tracker.h>
#include <pingdart/ui/runtime/component_instance.h>
#include <pingdart/ui/runtime/hook_types.h>

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace pingdart::ui {

template <typename T>
struct MemoCache {

    std::optional<T> value;

    HookDeps dependencies;
};

template <typename F>
auto useMemo(F&& compute, HookDeps deps)
    -> std::decay_t<std::invoke_result_t<F&>>
{
    using T = std::decay_t<std::invoke_result_t<F&>>;

    auto inst = DependencyTracker::active_instance();

    // Outside component rendering, calculate directly.
    if (!inst) {
        return std::invoke(compute);
    }

    // Each component and hook position owns its cache.
    auto& slot =
        inst->template next_hook<MemoCache<T>>();

    if (!slot.value || slot.dependencies != deps) {

        MemoCache<T> updated;

        updated.value.emplace(std::invoke(compute));
        updated.dependencies = std::move(deps);

        slot = std::move(updated);
    }

    return *slot.value;
}

} // namespace pingdart::ui
// include/pingdart/ui/hooks/use_effect.h
// PingDartUI React-like useEffect Hook (PKG-138)
#pragma once

#include <pingdart/ui/runtime/dependency_tracker.h>
#include <pingdart/ui/runtime/component_instance.h>
#include <functional>
#include <vector>
#include <string>
#include <type_traits>

namespace pingdart::ui {

using CleanupFn = std::function<void()>;

template <typename Fn>
inline void useEffect(Fn&& fn, const std::vector<std::string>& /*deps*/ = {}) {
    auto inst = DependencyTracker::active_instance();
    if (inst) {
        inst->add_mount_callback([inst, f = std::forward<Fn>(fn)]() {
            if constexpr (std::is_invocable_r_v<CleanupFn, Fn>) {
                auto cleanup = f();
                if (cleanup) {
                    inst->add_cleanup_callback(std::move(cleanup));
                }
            } else {
                f();
            }
        });
    } else {
        if constexpr (std::is_invocable_r_v<CleanupFn, Fn>) {
            auto cleanup = fn();
            (void)cleanup;
        } else {
            fn();
        }
    }
}

} // namespace pingdart::ui

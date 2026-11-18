// include/pingdart/ui/runtime/lifecycle.h
// PingDartUI Lifecycle Contracts & Resource Management (PKG-138)
#pragma once

#include <functional>
#include <vector>
#include <string>

namespace pingdart::ui {

    enum class LifecycleState {
        Unmounted,
        Mounting,
        Mounted,
        Updating,
        Unmounting
    };

    class LifecycleHook {
    public:
        virtual ~LifecycleHook() = default;
        virtual void on_mount() {}
        virtual void on_update() {}
        virtual void on_unmount() {}
    };

} // namespace pingdart::ui

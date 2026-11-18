// include/pingdart/ui/runtime/dependency_tracker.h
// PingDartUI Reactive Signal Dependency Tracker (PKG-138)
#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <unordered_set>
#include <mutex>

namespace pingdart::ui {

class ComponentInstance;

class DependencyTracker {
private:
    static inline thread_local std::shared_ptr<ComponentInstance> s_active_instance{nullptr};
    static inline thread_local std::vector<std::function<void()>>* s_active_subscriber_list{nullptr};

public:
    static void set_active_instance(std::shared_ptr<ComponentInstance> inst) {
        s_active_instance = inst;
    }

    static std::shared_ptr<ComponentInstance> active_instance() {
        return s_active_instance;
    }

    static void set_active_subscriber_list(std::vector<std::function<void()>>* list) {
        s_active_subscriber_list = list;
    }

    static std::vector<std::function<void()>>* active_subscriber_list() {
        return s_active_subscriber_list;
    }
};

} // namespace pingdart::ui

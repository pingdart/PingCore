// include/pingdart/ui/events/dispatcher.h
// PingDartUI Event Dispatcher with Capture, Target & Bubble Phases (PKG-138)
#pragma once

#include <pingdart/ui/events/event.h>
#include <pingdart/ui/runtime/component_instance.h>
#include <vector>
#include <algorithm>
#include <memory>

namespace pingdart::ui {

class EventDispatcher {
public:
    static EventDispatcher& instance() {
        static EventDispatcher s_dispatcher;
        return s_dispatcher;
    }

    static void dispatch(Event& event, const std::shared_ptr<ComponentInstance>& target) {
        instance().dispatch_event(event, target);
    }

    void dispatch_event(Event& event, const std::shared_ptr<ComponentInstance>& target) {
        if (!target) return;

        event.set_target(target);

        // 1. Build propagation path from root to target
        std::vector<std::shared_ptr<ComponentInstance>> path;
        auto current = target;
        while (current) {
            path.push_back(current);
            current = current->parent();
        }
        std::reverse(path.begin(), path.end());

        // 2. Capture Phase: Root down to target's parent
        event.set_phase(EventPhase::Capturing);
        if (path.size() > 1) {
            for (size_t i = 0; i < path.size() - 1; ++i) {
                if (event.is_propagation_stopped()) return;
                event.set_current_target(path[i]);
                path[i]->invoke_listeners(event, true);
            }
        }

        // 3. Target Phase: Target node itself
        if (event.is_propagation_stopped()) return;
        event.set_phase(EventPhase::AtTarget);
        auto tgt = path.back();
        event.set_current_target(tgt);
        tgt->invoke_listeners(event, true);
        if (!event.is_propagation_stopped()) {
            tgt->invoke_listeners(event, false);
        }

        // 4. Bubbling Phase: Target's parent up to root
        if (event.bubbles() && !event.is_propagation_stopped() && path.size() > 1) {
            event.set_phase(EventPhase::Bubbling);
            for (size_t i = path.size() - 2;; --i) {
                if (event.is_propagation_stopped()) return;
                event.set_current_target(path[i]);
                path[i]->invoke_listeners(event, false);
                if (i == 0) break;
            }
        }
    }
};

} // namespace pingdart::ui

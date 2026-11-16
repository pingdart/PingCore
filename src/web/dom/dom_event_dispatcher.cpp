#include "dom_event_dispatcher.h"

namespace pingdart::web {

void DomEventDispatcher::add_handler(DomNodeHandle node, DomEventType type, DomEventHandler handler) {
    if (node.is_valid() && handler) {
        handle_handlers_[node].push_back(HandlerEntry{type, std::move(handler)});
    }
}

void DomEventDispatcher::add_handler_by_id(const std::string& pd_id, DomEventType type, DomEventHandler handler) {
    if (!pd_id.empty() && handler) {
        id_handlers_[pd_id].push_back(HandlerEntry{type, std::move(handler)});
    }
}

void DomEventDispatcher::remove_handlers(DomNodeHandle node) {
    handle_handlers_.erase(node);
}

bool DomEventDispatcher::dispatch(const DomEvent& event) {
    bool handled = false;

    // 1. Direct handle lookup
    if (event.target.is_valid()) {
        auto it = handle_handlers_.find(event.target);
        if (it != handle_handlers_.end()) {
            for (const auto& entry : it->second) {
                if (entry.type == event.type) {
                    entry.handler(event);
                    handled = true;
                }
            }
        }
    }

    // 2. data-pd-id lookup
    if (!event.target_pd_id.empty()) {
        auto it = id_handlers_.find(event.target_pd_id);
        if (it != id_handlers_.end()) {
            for (const auto& entry : it->second) {
                if (entry.type == event.type) {
                    entry.handler(event);
                    handled = true;
                }
            }
        }
    }

    return handled;
}

} // namespace pingdart::web

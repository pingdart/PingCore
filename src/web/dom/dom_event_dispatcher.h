#pragma once

#include <pingdart/web/dom_event.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace pingdart::web {

class DomEventDispatcher {
public:
    DomEventDispatcher() = default;

    void add_handler(DomNodeHandle node, DomEventType type, DomEventHandler handler);
    void add_handler_by_id(const std::string& pd_id, DomEventType type, DomEventHandler handler);
    void remove_handlers(DomNodeHandle node);

    bool dispatch(const DomEvent& event);

    [[nodiscard]] size_t handler_count() const noexcept { return handle_handlers_.size() + id_handlers_.size(); }

private:
    struct HandlerEntry {
        DomEventType type;
        DomEventHandler handler;
    };

    std::unordered_map<DomNodeHandle, std::vector<HandlerEntry>> handle_handlers_;
    std::unordered_map<std::string, std::vector<HandlerEntry>> id_handlers_;
};

} // namespace pingdart::web

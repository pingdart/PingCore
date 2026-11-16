#pragma once

#include "dom_node_handle.h"
#include <string>
#include <cstdint>

namespace pingdart::web {

enum class DomEventType : uint8_t {
    Click,
    Input,
    Change,
    KeyDown,
    KeyUp,
    Focus,
    Blur,
    Submit
};

[[nodiscard]] inline std::string_view to_string(DomEventType type) noexcept {
    switch (type) {
        case DomEventType::Click:  return "click";
        case DomEventType::Input:  return "input";
        case DomEventType::Change: return "change";
        case DomEventType::KeyDown: return "keydown";
        case DomEventType::KeyUp:   return "keyup";
        case DomEventType::Focus:  return "focus";
        case DomEventType::Blur:   return "blur";
        case DomEventType::Submit: return "submit";
    }
    return "unknown";
}

struct DomEvent {
    DomEventType type;
    DomNodeHandle target{kInvalidDomNodeHandle};
    std::string target_pd_id;
    std::string value;       // For input/change
    std::string key;         // For keydown/keyup
    int32_t key_code{0};
    bool default_prevented{false};

    void prevent_default() noexcept { default_prevented = true; }
};

using DomEventHandler = std::function<void(const DomEvent&)>;

} // namespace pingdart::web

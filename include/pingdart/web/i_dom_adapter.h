#pragma once

#include "dom_node_handle.h"
#include "dom_event.h"
#include <string>
#include <string_view>
#include <vector>

namespace pingdart::web {

class IDomAdapter {
public:
    virtual ~IDomAdapter() = default;

    // Node Lifecycle
    [[nodiscard]] virtual DomNodeHandle create_element(std::string_view tag) = 0;
    [[nodiscard]] virtual DomNodeHandle create_text_node(std::string_view text) = 0;
    virtual void remove_node(DomNodeHandle node) = 0;

    // Tree Hierarchy
    virtual void append_child(DomNodeHandle parent, DomNodeHandle child) = 0;
    virtual void insert_before(DomNodeHandle parent, DomNodeHandle new_child, DomNodeHandle ref_child) = 0;
    virtual void remove_child(DomNodeHandle parent, DomNodeHandle child) = 0;

    // Attributes & Content
    virtual void set_attribute(DomNodeHandle node, std::string_view name, std::string_view value) = 0;
    virtual void remove_attribute(DomNodeHandle node, std::string_view name) = 0;
    virtual void set_text_content(DomNodeHandle node, std::string_view text) = 0;
    [[nodiscard]] virtual std::string get_attribute(DomNodeHandle node, std::string_view name) const = 0;
    [[nodiscard]] virtual std::string get_text_content(DomNodeHandle node) const = 0;
    [[nodiscard]] virtual std::string get_tag_name(DomNodeHandle node) const = 0;

    // Events & Focus
    virtual void add_event_listener(DomNodeHandle node, DomEventType type) = 0;
    virtual void remove_event_listener(DomNodeHandle node, DomEventType type) = 0;
    virtual void focus(DomNodeHandle node) = 0;
    [[nodiscard]] virtual DomNodeHandle active_element() const noexcept = 0;
};

} // namespace pingdart::web

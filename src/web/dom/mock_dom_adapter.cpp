#include "mock_dom_adapter.h"
#include <algorithm>

namespace pingdart::web {

DomNodeHandle MockDomAdapter::create_element(std::string_view tag) {
    DomNodeHandle handle{next_id_++};
    MockDomNode node;
    node.handle = handle;
    node.tag = std::string(tag);
    nodes_[handle] = std::move(node);
    return handle;
}

DomNodeHandle MockDomAdapter::create_text_node(std::string_view text) {
    DomNodeHandle handle{next_id_++};
    MockDomNode node;
    node.handle = handle;
    node.tag = "#text";
    node.text_content = std::string(text);
    nodes_[handle] = std::move(node);
    return handle;
}

void MockDomAdapter::remove_node(DomNodeHandle node) {
    nodes_.erase(node);
    if (active_element_ == node) {
        active_element_ = kInvalidDomNodeHandle;
    }
}

void MockDomAdapter::append_child(DomNodeHandle parent, DomNodeHandle child) {
    auto it = nodes_.find(parent);
    if (it != nodes_.end()) {
        it->second.children.push_back(child);
    }
}

void MockDomAdapter::insert_before(DomNodeHandle parent, DomNodeHandle new_child, DomNodeHandle ref_child) {
    auto it = nodes_.find(parent);
    if (it != nodes_.end()) {
        auto& children = it->second.children;
        auto pos = std::find(children.begin(), children.end(), ref_child);
        if (pos != children.end()) {
            children.insert(pos, new_child);
        } else {
            children.push_back(new_child);
        }
    }
}

void MockDomAdapter::remove_child(DomNodeHandle parent, DomNodeHandle child) {
    auto it = nodes_.find(parent);
    if (it != nodes_.end()) {
        auto& children = it->second.children;
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
    }
}

void MockDomAdapter::set_attribute(DomNodeHandle node, std::string_view name, std::string_view value) {
    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        it->second.attributes[std::string(name)] = std::string(value);
    }
}

void MockDomAdapter::remove_attribute(DomNodeHandle node, std::string_view name) {
    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        it->second.attributes.erase(std::string(name));
    }
}

void MockDomAdapter::set_text_content(DomNodeHandle node, std::string_view text) {
    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        it->second.text_content = std::string(text);
    }
}

std::string MockDomAdapter::get_attribute(DomNodeHandle node, std::string_view name) const {
    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        auto attr_it = it->second.attributes.find(std::string(name));
        if (attr_it != it->second.attributes.end()) {
            return attr_it->second;
        }
    }
    return "";
}

std::string MockDomAdapter::get_text_content(DomNodeHandle node) const {
    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        return it->second.text_content;
    }
    return "";
}

std::string MockDomAdapter::get_tag_name(DomNodeHandle node) const {
    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        return it->second.tag;
    }
    return "";
}

void MockDomAdapter::add_event_listener(DomNodeHandle node, DomEventType type) {
    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        it->second.listeners.insert(type);
    }
}

void MockDomAdapter::remove_event_listener(DomNodeHandle node, DomEventType type) {
    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        it->second.listeners.erase(type);
    }
}

void MockDomAdapter::focus(DomNodeHandle node) {
    if (nodes_.find(node) != nodes_.end()) {
        active_element_ = node;
    }
}

bool MockDomAdapter::has_listener(DomNodeHandle node, DomEventType type) const {
    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        return it->second.listeners.find(type) != it->second.listeners.end();
    }
    return false;
}

const std::vector<DomNodeHandle>& MockDomAdapter::get_children(DomNodeHandle parent) const {
    auto it = nodes_.find(parent);
    if (it != nodes_.end()) {
        return it->second.children;
    }
    return kEmptyChildren;
}

} // namespace pingdart::web

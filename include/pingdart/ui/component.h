// include/pingdart/ui/component.h
// PingDartUI Value-Type Component & Virtual Tree System (PKG-135)
#pragma once

#include <pingdart/ui/style.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <initializer_list>

namespace pingdart::ui {

struct ComponentNode;

class Component : public StyleBuilder<Component> {
private:
    std::shared_ptr<ComponentNode> node_;

public:
    Component();
    explicit Component(std::string type);
    Component(std::string type, std::string text);
    Component(std::string type, std::function<std::string()> reactive_text_fn);
    Component(std::string type, std::initializer_list<Component> child_list);
    Component(std::string type, std::vector<Component> child_vec);
    Component(std::initializer_list<Component> child_list);

    const std::string& type() const;
    std::string text() const;
    const std::vector<Component>& children() const;
    std::vector<Component>& children();
    const std::unordered_map<std::string, std::string>& attributes() const;

    // Event Listeners
    Component& onClick(std::function<void()> cb);
    Component& onChange(std::function<void(const std::string&)> cb);
    void emit_click() const;
    void emit_change(const std::string& val) const;

    // HTML / DOM Attributes
    Component& attr(const std::string& key, const std::string& val);
    Component& href(const std::string& url);
    Component& src(const std::string& url);
    Component& alt(const std::string& text);
    Component& placeholder(const std::string& text);

    // Style sync
    const ComputedStyle& style() const {
        return this->computed_style();
    }
};

struct ComponentNode {
    std::string type{"div"};
    std::string text_content{""};
    std::function<std::string()> reactive_text{nullptr};
    std::vector<Component> children{};
    ComputedStyle style{};
    std::unordered_map<std::string, std::string> attributes{};
    std::function<void()> on_click{nullptr};
    std::function<void(const std::string&)> on_change{nullptr};

    ComponentNode() = default;
    explicit ComponentNode(std::string t) : type(std::move(t)) {}
};

// Inline implementations after ComponentNode is complete
inline Component::Component() : node_(std::make_shared<ComponentNode>()) {}

inline Component::Component(std::string type)
    : node_(std::make_shared<ComponentNode>(std::move(type))) {}

inline Component::Component(std::string type, std::string text)
    : node_(std::make_shared<ComponentNode>(std::move(type))) {
    node_->text_content = std::move(text);
}

inline Component::Component(std::string type, std::function<std::string()> reactive_text_fn)
    : node_(std::make_shared<ComponentNode>(std::move(type))) {
    node_->reactive_text = std::move(reactive_text_fn);
}

inline Component::Component(std::string type, std::initializer_list<Component> child_list)
    : node_(std::make_shared<ComponentNode>(std::move(type))) {
    node_->children.assign(child_list.begin(), child_list.end());
}

inline Component::Component(std::string type, std::vector<Component> child_vec)
    : node_(std::make_shared<ComponentNode>(std::move(type))) {
    node_->children = std::move(child_vec);
}

inline Component::Component(std::initializer_list<Component> child_list)
    : node_(std::make_shared<ComponentNode>("div")) {
    node_->children.assign(child_list.begin(), child_list.end());
}

inline const std::string& Component::type() const {
    return node_->type;
}

inline std::string Component::text() const {
    if (node_->reactive_text) {
        return node_->reactive_text();
    }
    return node_->text_content;
}

inline const std::vector<Component>& Component::children() const {
    return node_->children;
}

inline std::vector<Component>& Component::children() {
    return node_->children;
}

inline const std::unordered_map<std::string, std::string>& Component::attributes() const {
    return node_->attributes;
}

inline Component& Component::onClick(std::function<void()> cb) {
    node_->on_click = std::move(cb);
    return *this;
}

inline Component& Component::onChange(std::function<void(const std::string&)> cb) {
    node_->on_change = std::move(cb);
    return *this;
}

inline void Component::emit_click() const {
    if (node_->on_click) {
        node_->on_click();
    }
}

inline void Component::emit_change(const std::string& val) const {
    if (node_->on_change) {
        node_->on_change(val);
    }
}

inline Component& Component::attr(const std::string& key, const std::string& val) {
    node_->attributes[key] = val;
    return *this;
}

inline Component& Component::href(const std::string& url) {
    return attr("href", url);
}

inline Component& Component::src(const std::string& url) {
    return attr("src", url);
}

inline Component& Component::alt(const std::string& text) {
    return attr("alt", text);
}

inline Component& Component::placeholder(const std::string& text) {
    return attr("placeholder", text);
}

} // namespace pingdart::ui

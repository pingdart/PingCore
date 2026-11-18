// include/pingdart/ui/component.h
// PingDartUI Value-Type Component & Virtual Tree System (PKG-136)
#pragma once

#include <pingdart/ui/style.h>
#include <pingdart/ui/css_parser.h>
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

    // Stable Identity Key
    const std::string& key() const;
    Component& key(std::string k);

    // Accessibility Semantics
    Component& role(const std::string& r);
    Component& ariaLabel(const std::string& label);
    Component& ariaHidden(bool hidden = true);

    // Interactive States & Pseudo Overrides
    Component& hover(const ComputedStyle& hover_style);
    Component& focus(const ComputedStyle& focus_style);
    Component& active(const ComputedStyle& active_style);
    Component& disabled(bool is_dis = true);
    Component& checked(bool is_chk = true);

    bool is_disabled() const;
    bool is_checked() const;
    bool is_hovered() const;
    bool is_focused() const;

    // Event Listeners
    Component& onClick(std::function<void()> cb);
    Component& onChange(std::function<void(const std::string&)> cb);
    void emit_click() const;
    void emit_change(const std::string& val) const;

    // HTML / DOM Attributes
    Component& id(const std::string& elem_id);
    const std::string& id() const;

    Component& attr(const std::string& key, const std::string& val);
    Component& href(const std::string& url);
    Component& src(const std::string& url);
    Component& alt(const std::string& text);
    Component& placeholder(const std::string& text);
    Component& value(const std::string& val);

    // Style sync & cascade
    const ComputedStyle& style() const;
    void apply_css_rules(const std::vector<CssRule>& rules);
};

struct ComponentNode {
    std::string type{"div"};
    std::string id{""};
    std::string key{""};
    std::string text_content{""};
    std::function<std::string()> reactive_text{nullptr};
    std::vector<Component> children{};
    ComputedStyle style{};
    std::unordered_map<std::string, std::string> attributes{};
    std::function<void()> on_click{nullptr};
    std::function<void(const std::string&)> on_change{nullptr};

    bool is_disabled{false};
    bool is_hovered{false};
    bool is_focused{false};

    ComponentNode() = default;
    explicit ComponentNode(std::string t) : type(std::move(t)) {}
};

// Inline implementations
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

inline const std::string& Component::key() const {
    return node_->key;
}

inline Component& Component::key(std::string k) {
    node_->key = std::move(k);
    return *this;
}

inline Component& Component::id(const std::string& elem_id) {
    node_->id = elem_id;
    node_->attributes["id"] = elem_id;
    return *this;
}

inline const std::string& Component::id() const {
    return node_->id;
}

inline Component& Component::role(const std::string& r) {
    node_->attributes["role"] = r;
    return *this;
}

inline Component& Component::ariaLabel(const std::string& label) {
    node_->attributes["aria-label"] = label;
    return *this;
}

inline Component& Component::ariaHidden(bool hidden) {
    node_->attributes["aria-hidden"] = hidden ? "true" : "false";
    return *this;
}

inline Component& Component::hover(const ComputedStyle& hover_style) {
    style_.hover_style = std::make_shared<ComputedStyle>(hover_style);
    return *this;
}

inline Component& Component::focus(const ComputedStyle& focus_style) {
    style_.focus_style = std::make_shared<ComputedStyle>(focus_style);
    return *this;
}

inline Component& Component::active(const ComputedStyle& active_style) {
    style_.active_style = std::make_shared<ComputedStyle>(active_style);
    return *this;
}

inline Component& Component::disabled(bool is_dis) {
    node_->is_disabled = is_dis;
    if (is_dis) {
        node_->attributes["disabled"] = "disabled";
        style_.opacity = 0.5f;
    } else {
        node_->attributes.erase("disabled");
        style_.opacity = 1.0f;
    }
    return *this;
}

inline Component& Component::checked(bool is_chk) {
    if (is_chk) {
        node_->attributes["checked"] = "checked";
    } else {
        node_->attributes.erase("checked");
    }
    return *this;
}

inline bool Component::is_checked() const {
    return node_->attributes.find("checked") != node_->attributes.end();
}

inline bool Component::is_disabled() const {
    return node_->is_disabled;
}

inline bool Component::is_hovered() const {
    return node_->is_hovered;
}

inline bool Component::is_focused() const {
    return node_->is_focused;
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
    if (!node_->is_disabled && node_->on_click) {
        node_->on_click();
    }
}

inline void Component::emit_change(const std::string& val) const {
    if (!node_->is_disabled && node_->on_change) {
        node_->on_change(val);
    }
}

inline Component& Component::attr(const std::string& k, const std::string& v) {
    node_->attributes[k] = v;
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

inline Component& Component::value(const std::string& val) {
    return attr("value", val);
}

inline const ComputedStyle& Component::style() const {
    return this->computed_style();
}

inline void Component::apply_css_rules(const std::vector<CssRule>& rules) {
    // Determine current pseudo state if any
    std::string current_pseudo = "";
    if (node_->is_disabled) current_pseudo = "disabled";
    else if (node_->is_hovered) current_pseudo = "hover";
    else if (node_->is_focused) current_pseudo = "focus";

    // Sort matching rules by specificity and declaration order
    std::vector<const CssRule*> matching_rules;
    for (const auto& rule : rules) {
        for (const auto& sel : rule.selectors) {
            if (sel.matches(node_->type, style_.class_names, node_->id, current_pseudo)) {
                matching_rules.push_back(&rule);
                break;
            }
        }
    }

    std::sort(matching_rules.begin(), matching_rules.end(), [](const CssRule* r1, const CssRule* r2) {
        if (r1->specificity == r2->specificity) {
            return r1->order < r2->order;
        }
        return r1->specificity < r2->specificity;
    });

    auto& theme = ThemeEngine::instance();

    for (const auto* rule : matching_rules) {
        for (const auto& decl : rule->declarations) {
            std::string resolved_val = CssParser::resolve_variables(decl.value, theme);

            if (decl.property == "background" || decl.property == "background-color") {
                if (!style_.important.background_color || decl.is_important) {
                    style_.background_color = resolved_val;
                    if (decl.is_important) style_.important.background_color = true;
                }
            } else if (decl.property == "color") {
                if (!style_.important.text_color || decl.is_important) {
                    style_.text_color = resolved_val;
                    if (decl.is_important) style_.important.text_color = true;
                }
            } else if (decl.property == "border-radius") {
                if (!style_.important.border_radius || decl.is_important) {
                    style_.border_radius = CssParser::parse_px_dimension(resolved_val);
                    if (decl.is_important) style_.important.border_radius = true;
                }
            } else if (decl.property == "padding") {
                if (!style_.important.padding || decl.is_important) {
                    float p = CssParser::parse_px_dimension(resolved_val);
                    style_.padding = Insets(p);
                    if (decl.is_important) style_.important.padding = true;
                }
            } else if (decl.property == "margin") {
                if (!style_.important.margin || decl.is_important) {
                    float m = CssParser::parse_px_dimension(resolved_val);
                    style_.margin = Insets(m);
                    if (decl.is_important) style_.important.margin = true;
                }
            } else if (decl.property == "gap") {
                if (!style_.important.gap || decl.is_important) {
                    style_.gap = CssParser::parse_px_dimension(resolved_val);
                    if (decl.is_important) style_.important.gap = true;
                }
            } else if (decl.property == "font-size") {
                if (!style_.important.font_size || decl.is_important) {
                    style_.font_size = CssParser::parse_px_dimension(resolved_val);
                    if (decl.is_important) style_.important.font_size = true;
                }
            } else if (decl.property == "border") {
                // e.g. "1px solid #e2e8f0"
                std::istringstream b_iss(resolved_val);
                std::string b_w, b_s, b_c;
                b_iss >> b_w >> b_s >> b_c;
                if (!style_.important.border || decl.is_important) {
                    style_.border_width = CssParser::parse_px_dimension(b_w);
                    if (!b_s.empty()) style_.border_style = b_s;
                    if (!b_c.empty()) style_.border_color = b_c;
                    if (decl.is_important) style_.important.border = true;
                }
            }
        }
    }

    // Recursively apply to children
    for (auto& child : node_->children) {
        child.apply_css_rules(rules);
    }
}

} // namespace pingdart::ui

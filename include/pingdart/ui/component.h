// include/pingdart/ui/component.h
// PingDartUI Value-Type Component & Virtual Tree System (PKG-136)
#pragma once

#include <pingdart/ui/style.h>
#include <pingdart/ui/css_parser.h>
#include <pingdart/ui/gestures/swipe.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <initializer_list>

namespace pingdart::ui {

// Form Submission Key-Value Container
class FormData {
private:
    std::unordered_map<std::string, std::string> data_{};

public:
    FormData() = default;

    void set(const std::string& key, const std::string& val) {
        data_[key] = val;
    }

    std::string get(const std::string& key, const std::string& fallback = "") const {
        auto it = data_.find(key);
        if (it != data_.end()) return it->second;
        return fallback;
    }

    bool has(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

    const std::unordered_map<std::string, std::string>& all() const {
        return data_;
    }
};

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
    Component& onDoubleClick(std::function<void()> cb);
    Component& onChange(std::function<void(const std::string&)> cb);
    Component& onSubmit(std::function<void(const FormData&)> cb);
    Component& onHover(std::function<void(bool)> cb);
    Component& onFocus(std::function<void()> cb);
    Component& onBlur(std::function<void()> cb);

    // Gestures
    Component& onTap(std::function<void()> cb);
    Component& onLongPress(std::function<void()> cb);
    Component& onSwipe(std::function<void(Direction)> cb);
    Component& onPan(std::function<void(float dx, float dy)> cb);
    Component& onPinch(std::function<void(float scale)> cb);

    // Lifecycle
    Component& onMount(std::function<void()> cb);
    Component& onUpdate(std::function<void()> cb);
    Component& onUnmount(std::function<void()> cb);

    void emit_click() const;
    void emit_double_click() const;
    void emit_change(const std::string& val) const;
    void emit_submit(const FormData& data) const;
    void emit_hover(bool hovered);
    void emit_focus();
    void emit_blur();
    void emit_tap() const;
    void emit_long_press() const;
    void emit_swipe(Direction dir) const;
    void emit_pan(float dx, float dy) const;
    void emit_pinch(float scale) const;

    // HTML / DOM Attributes
    Component& id(const std::string& elem_id);
    const std::string& id() const;

    Component& attr(const std::string& key, const std::string& val);
    Component& href(const std::string& url);
    Component& src(const std::string& url);
    Component& alt(const std::string& text);
    Component& placeholder(const std::string& text);
    Component& value(const std::string& val);
    Component& type(const std::string& t);

    // Scoped Theme Context
    Component& theme(std::shared_ptr<ThemeContext> ctx);
    std::shared_ptr<ThemeContext> theme() const;

    // Style sync & cascade
    const ComputedStyle& style() const;
    void apply_css_rules(const std::vector<CssRule>& rules);

    // Layout Geometry Engine
    void compute_layout(float parent_x = 0.0f, float parent_y = 0.0f, float parent_w = 800.0f, float parent_h = 600.0f);
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
    std::function<void()> on_double_click{nullptr};
    std::function<void(const std::string&)> on_change{nullptr};

    bool is_disabled{false};
    bool is_hovered{false};
    bool is_focused{false};

    std::function<void(const FormData&)> on_submit{nullptr};
    std::function<void(bool)> on_hover{nullptr};
    std::function<void()> on_focus{nullptr};
    std::function<void()> on_blur{nullptr};
    std::shared_ptr<ThemeContext> local_theme{nullptr};

    // Gestures & Lifecycle
    std::function<void()> on_tap{nullptr};
    std::function<void()> on_long_press{nullptr};
    std::function<void(Direction)> on_swipe{nullptr};
    std::function<void(float, float)> on_pan{nullptr};
    std::function<void(float)> on_pinch{nullptr};
    std::function<void()> on_mount{nullptr};
    std::function<void()> on_update{nullptr};
    std::function<void()> on_unmount{nullptr};

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

inline Component& Component::onDoubleClick(std::function<void()> cb) {
    node_->on_double_click = std::move(cb);
    return *this;
}

inline Component& Component::onChange(std::function<void(const std::string&)> cb) {
    node_->on_change = std::move(cb);
    return *this;
}

inline Component& Component::onSubmit(std::function<void(const FormData&)> cb) {
    node_->on_submit = std::move(cb);
    return *this;
}

inline Component& Component::onHover(std::function<void(bool)> cb) {
    node_->on_hover = std::move(cb);
    return *this;
}

inline Component& Component::onFocus(std::function<void()> cb) {
    node_->on_focus = std::move(cb);
    return *this;
}

inline Component& Component::onBlur(std::function<void()> cb) {
    node_->on_blur = std::move(cb);
    return *this;
}

inline Component& Component::onTap(std::function<void()> cb) {
    node_->on_tap = std::move(cb);
    return *this;
}

inline Component& Component::onLongPress(std::function<void()> cb) {
    node_->on_long_press = std::move(cb);
    return *this;
}

inline Component& Component::onSwipe(std::function<void(Direction)> cb) {
    node_->on_swipe = std::move(cb);
    return *this;
}

inline Component& Component::onPan(std::function<void(float, float)> cb) {
    node_->on_pan = std::move(cb);
    return *this;
}

inline Component& Component::onPinch(std::function<void(float)> cb) {
    node_->on_pinch = std::move(cb);
    return *this;
}

inline Component& Component::onMount(std::function<void()> cb) {
    node_->on_mount = std::move(cb);
    return *this;
}

inline Component& Component::onUpdate(std::function<void()> cb) {
    node_->on_update = std::move(cb);
    return *this;
}

inline Component& Component::onUnmount(std::function<void()> cb) {
    node_->on_unmount = std::move(cb);
    return *this;
}

inline void Component::emit_click() const {
    if (!node_->is_disabled && node_->on_click) {
        node_->on_click();
    }
}

inline void Component::emit_double_click() const {
    if (!node_->is_disabled && node_->on_double_click) {
        node_->on_double_click();
    }
}

inline void Component::emit_tap() const {
    if (!node_->is_disabled) {
        if (node_->on_tap) node_->on_tap();
        else if (node_->on_click) node_->on_click();
    }
}

inline void Component::emit_long_press() const {
    if (!node_->is_disabled && node_->on_long_press) {
        node_->on_long_press();
    }
}

inline void Component::emit_swipe(Direction dir) const {
    if (!node_->is_disabled && node_->on_swipe) {
        node_->on_swipe(dir);
    }
}

inline void Component::emit_pan(float dx, float dy) const {
    if (!node_->is_disabled && node_->on_pan) {
        node_->on_pan(dx, dy);
    }
}

inline void Component::emit_pinch(float scale) const {
    if (!node_->is_disabled && node_->on_pinch) {
        node_->on_pinch(scale);
    }
}

inline void Component::emit_change(const std::string& val) const {
    if (!node_->is_disabled && node_->on_change) {
        node_->on_change(val);
    }
}

inline void Component::emit_submit(const FormData& data) const {
    if (!node_->is_disabled && node_->on_submit) {
        node_->on_submit(data);
    }
}

inline void Component::emit_hover(bool hovered) {
    node_->is_hovered = hovered;
    if (node_->on_hover) {
        node_->on_hover(hovered);
    }
}

inline void Component::emit_focus() {
    node_->is_focused = true;
    if (node_->on_focus) {
        node_->on_focus();
    }
}

inline void Component::emit_blur() {
    node_->is_focused = false;
    if (node_->on_blur) {
        node_->on_blur();
    }
}

inline Component& Component::type(const std::string& t) {
    node_->type = t;
    return *this;
}

inline Component& Component::theme(std::shared_ptr<ThemeContext> ctx) {
    node_->local_theme = std::move(ctx);
    return *this;
}

inline std::shared_ptr<ThemeContext> Component::theme() const {
    return node_->local_theme;
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

inline void Component::compute_layout(float parent_x, float parent_y, float parent_w, float parent_h) {
    float x = parent_x + style_.margin.left;
    float y = parent_y + style_.margin.top;

    float w = parent_w;
    if (style_.width == "100%") {
        w = std::max(0.0f, parent_w - style_.margin.left - style_.margin.right);
    } else if (!style_.width.empty()) {
        w = CssParser::parse_px_dimension(style_.width);
    }
    if (style_.max_width > 0 && w > style_.max_width) {
        w = style_.max_width;
    }
    if (style_.min_width > 0 && w < style_.min_width) {
        w = style_.min_width;
    }

    float h = parent_h;
    if (style_.height == "100%") {
        h = std::max(0.0f, parent_h - style_.margin.top - style_.margin.bottom);
    } else if (!style_.height.empty()) {
        h = CssParser::parse_px_dimension(style_.height);
    }
    if (style_.max_height > 0 && h > style_.max_height) {
        h = style_.max_height;
    }
    if (style_.min_height > 0 && h < style_.min_height) {
        h = style_.min_height;
    }

    style_.layout_bounds = Rect{x, y, w, h};

    if (node_->children.empty()) return;

    if (style_.flex_direction == FlexDirection::Row) {
        float cur_x = x + style_.padding.left;
        float cur_y = y + style_.padding.top;
        float inner_w = std::max(0.0f, w - style_.padding.left - style_.padding.right);
        float total_gaps = static_cast<float>(node_->children.size() - 1) * style_.gap;
        float child_avail_w = std::max(0.0f, (inner_w - total_gaps) / static_cast<float>(node_->children.size()));
        float child_avail_h = std::max(0.0f, h - style_.padding.top - style_.padding.bottom);

        for (auto& child : node_->children) {
            float cw = child.style().width.empty() ? child_avail_w : CssParser::parse_px_dimension(child.style().width);
            float ch = child.style().height.empty() ? child_avail_h : CssParser::parse_px_dimension(child.style().height);
            child.compute_layout(cur_x, cur_y, cw, ch);
            cur_x += child.style().layout_bounds.width + style_.gap;
        }
    } else {
        float cur_x = x + style_.padding.left;
        float cur_y = y + style_.padding.top;
        float child_avail_w = std::max(0.0f, w - style_.padding.left - style_.padding.right);

        for (auto& child : node_->children) {
            float cw = child.style().width.empty() ? child_avail_w : (child.style().width == "100%" ? child_avail_w : CssParser::parse_px_dimension(child.style().width));
            float ch = child.style().height.empty() ? 32.0f : CssParser::parse_px_dimension(child.style().height);
            child.compute_layout(cur_x, cur_y, cw, ch);
            cur_y += child.style().layout_bounds.height + style_.gap;
        }
    }
}

} // namespace pingdart::ui

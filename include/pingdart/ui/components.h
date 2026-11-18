// include/pingdart/ui/components.h
// PingDartUI Production Component Library (PKG-136)
// First 25 Core Components in PKG-137A Dependency Order:
// Layout (7), Basic (6), Actions (2), Forms (6), Display (3), Overlay (1)
#pragma once

#include <pingdart/ui/component.h>
#include <pingdart/ui/router.h>
#include <string>
#include <vector>
#include <initializer_list>
#include <functional>
#include <algorithm>

namespace pingdart::ui {

// ============================================================================
// 1. Layout Components (7)
// ============================================================================

// 1.1 Container — Primary flexible container with column flow by default
inline Component Container(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .flexDirection(FlexDirection::Column)
        .width("100%");
}

inline Component Container(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Column)
        .width("100%");
}

// 1.2 Row — Horizontal flex layout with centered vertical alignment
inline Component Row(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center);
}

inline Component Row(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center);
}

// 1.3 Column — Vertical flex layout
inline Component Column(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .flexDirection(FlexDirection::Column);
}

inline Component Column(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Column);
}

// 1.4 Stack — Layered container for z-index or absolute overlay positioning
inline Component Stack(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .position("relative")
        .display("flex")
        .flexDirection(FlexDirection::Column);
}

inline Component Stack(std::vector<Component> children) {
    return Component("div", std::move(children))
        .position("relative")
        .display("flex")
        .flexDirection(FlexDirection::Column);
}

// 1.5 Flex — Generic flex container with full layout property control
inline Component Flex(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .display("flex");
}

inline Component Flex(std::vector<Component> children) {
    return Component("div", std::move(children))
        .display("flex");
}

// 1.6 Grid — CSS Grid layout container
inline Component Grid(const std::string& cols, float gap_px, std::initializer_list<Component> children) {
    return Component("div", children)
        .grid(cols, gap_px);
}

inline Component Grid(const std::string& cols, float gap_px, std::vector<Component> children) {
    return Component("div", std::move(children))
        .grid(cols, gap_px);
}

inline Component Grid(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .grid("repeat(auto-fit, minmax(200px, 1fr))", 16.0f);
}

inline Component Grid(std::vector<Component> children) {
    return Component("div", std::move(children))
        .grid("repeat(auto-fit, minmax(200px, 1fr))", 16.0f);
}

// 1.7 ScrollView — Scrollable container viewport with overflow clipping
inline Component ScrollView(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .overflowY("auto")
        .flexDirection(FlexDirection::Column)
        .width("100%")
        .maxHeight(600.0f);
}

inline Component ScrollView(std::vector<Component> children) {
    return Component("div", std::move(children))
        .overflowY("auto")
        .flexDirection(FlexDirection::Column)
        .width("100%")
        .maxHeight(600.0f);
}

// ============================================================================
// 2. Basic Components (6)
// ============================================================================

// 2.1 Text — Semantic text span or reactive string binding
inline Component Text(const std::string& content) {
    return Component("span", content);
}

inline Component Text(std::function<std::string()> reactive_content) {
    return Component("span", std::move(reactive_content));
}

// 2.2 Heading — Semantic heading (h1–h6) with proportional typographic hierarchy
inline Component Heading(const std::string& content, int level = 1) {
    int lvl = std::clamp(level, 1, 6);
    std::string tag = "h" + std::to_string(lvl);
    auto comp = Component(tag, content);
    if (lvl == 1) comp.fontSize(32).weight(FontWeight::Bold);
    else if (lvl == 2) comp.fontSize(24).weight(FontWeight::Bold);
    else if (lvl == 3) comp.fontSize(20).weight(FontWeight::SemiBold);
    else if (lvl == 4) comp.fontSize(18).weight(FontWeight::SemiBold);
    else comp.fontSize(16).weight(FontWeight::Medium);
    return comp;
}

// 2.3 Image — Image tag with src, alt, and display formatting
inline Component Image(const std::string& src, const std::string& alt = "") {
    return Component("img").src(src).alt(alt);
}

// 2.4 Icon — Accessible icon tag with data-icon and icon class name
inline Component Icon(const std::string& name) {
    return Component("i").attr("data-icon", name).className("icon-" + name);
}

// 2.5 Divider — Horizontal rule separator
inline Component Divider() {
    return Component("hr")
        .width("100%")
        .height(1)
        .background("#E2E8F0")
        .border(0, "transparent");
}

// 2.6 Spacer — Invisible layout spacer box
inline Component Spacer(float size = 16.0f) {
    return Component("div").width(size).height(size);
}

// ============================================================================
// 3. Actions (2)
// ============================================================================

// 3.1 Button — Interactive trigger button
inline Component Button(const std::string& label) {
    return Component("button", label)
        .padding(10, 18)
        .borderRadius(8)
        .fontSize(15)
        .weight(FontWeight::Medium)
        .background("#F1F5F9")
        .color("#1E293B")
        .cursor("pointer");
}

// 3.2 IconButton — Button containing an icon with accessible label
inline Component IconButton(const std::string& icon_name, const std::string& label = "") {
    return Component("button", { Icon(icon_name) })
        .attr("aria-label", label.empty() ? icon_name : label)
        .padding(8, 8)
        .borderRadius(8)
        .background("#F8FAFC")
        .border(1, "#E2E8F0")
        .cursor("pointer");
}

// ============================================================================
// 4. Forms (6)
// ============================================================================

// 4.1 Input — Text entry field
inline Component Input(const std::string& placeholder = "", const std::string& val = "") {
    auto comp = Component("input")
        .placeholder(placeholder)
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
    comp.attr("type", "text");
    if (!val.empty()) comp.value(val);
    return comp;
}

// 4.2 TextArea — Multi-line text entry field
inline Component TextArea(const std::string& placeholder = "", int rows = 4) {
    return Component("textarea")
        .placeholder(placeholder)
        .attr("rows", std::to_string(rows))
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
}

// 4.3 Checkbox — Boolean toggle checkbox with label
inline Component Checkbox(const std::string& label, bool is_checked = false) {
    auto input_comp = Component("input")
        .attr("type", "checkbox")
        .checked(is_checked);
    return Component("label", { input_comp, Text(label) })
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center)
        .gap(8)
        .cursor("pointer");
}

// 4.4 Radio — Radio option button with group name and value
inline Component Radio(const std::string& name, const std::string& val, const std::string& label, bool is_checked = false) {
    auto input_comp = Component("input")
        .attr("type", "radio")
        .attr("name", name)
        .value(val)
        .checked(is_checked);
    return Component("label", { input_comp, Text(label) })
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center)
        .gap(8)
        .cursor("pointer");
}

// 4.5 Switch — Accessible toggle switch component
inline Component Switch(bool is_checked = false) {
    auto thumb = Component("div")
        .width(18)
        .height(18)
        .borderRadius(9999)
        .background("#FFFFFF")
        .shadow(0, 1, 2, "#00000026");
    if (is_checked) {
        thumb.margin(0, 0, 0, 18);
    }

    return Component("button", { thumb })
        .role("switch")
        .attr("aria-checked", is_checked ? "true" : "false")
        .width(42)
        .height(24)
        .borderRadius(9999)
        .background(is_checked ? "#2563EB" : "#CBD5E1")
        .border(0, "transparent")
        .padding(3)
        .cursor("pointer");
}

// 4.6 Option & Select — Dropdown menu
inline Component Option(const std::string& label, const std::string& val = "") {
    auto opt = Component("option", label);
    opt.value(val.empty() ? label : val);
    return opt;
}

inline Component Select(std::initializer_list<Component> options) {
    return Component("select", options)
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
}

inline Component Select(std::vector<Component> options) {
    return Component("select", std::move(options))
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
}

// ============================================================================
// 5. Display (3)
// ============================================================================

// 5.1 Card — Elevated container surface
inline Component Card(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .flexDirection(FlexDirection::Column)
        .background("#FFFFFF")
        .borderRadius(16)
        .border(1, "#E2E8F0")
        .padding(20)
        .gap(12)
        .shadow(0, 4, 6, "#0000000D");
}

inline Component Card(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Column)
        .background("#FFFFFF")
        .borderRadius(16)
        .border(1, "#E2E8F0")
        .padding(20)
        .gap(12)
        .shadow(0, 4, 6, "#0000000D");
}

// 5.2 Avatar — User or entity circular avatar with image or fallback initials
inline Component Avatar(const std::string& src_or_initials, bool is_image = false, float size = 40.0f) {
    if (is_image) {
        return Component("img")
            .src(src_or_initials)
            .width(size)
            .height(size)
            .borderRadius(9999)
            .attr("style", "object-fit: cover;");
    } else {
        return Component("div", { Text(src_or_initials).weight(FontWeight::SemiBold).fontSize(size * 0.4f).color("#1E293B") })
            .width(size)
            .height(size)
            .borderRadius(9999)
            .background("#E2E8F0")
            .display("flex")
            .alignItems(AlignItems::Center)
            .justifyContent(JustifyContent::Center);
    }
}

// 5.3 Badge — Compact status pill
inline Component Badge(const std::string& label) {
    return Component("span", label)
        .padding(4, 10)
        .borderRadius(9999)
        .fontSize(12)
        .weight(FontWeight::Medium)
        .background("#EFF6FF")
        .color("#2563EB");
}

// ============================================================================
// 6. Overlay (1)
// ============================================================================

// 6.1 Dialog — Modal overlay dialog with backdrop and central card
inline Component Dialog(const std::string& title, const Component& body, bool is_open = true) {
    if (!is_open) {
        return Component("div").attr("aria-hidden", "true").display("none");
    }

    auto modal_content = Card({
        Row({
            Heading(title, 3),
            Spacer(),
            IconButton("close", "Close dialog")
        }),
        Divider(),
        body
    }).width("480px").shadow(0, 20, 25, -5, "#0000001A");

    return Component("div", { modal_content })
        .role("dialog")
        .attr("aria-modal", "true")
        .position("relative")
        .padding(32)
        .background("#00000033")
        .display("flex")
        .alignItems(AlignItems::Center)
        .justifyContent(JustifyContent::Center);
}

// ============================================================================
// Navigation & Feedback Helpers
// ============================================================================

inline Component Link(const std::string& label, const std::string& href) {
    return Component("a", label)
        .href(href)
        .color("#2563EB")
        .onClick([href]() {
            Router::push(href);
        });
}

inline Component Alert(const std::string& message) {
    return Component("div", { Text(message) })
        .padding(14, 18)
        .borderRadius(8)
        .background("#FEF2F2")
        .border(1, "#FECACA")
        .color("#991B1B");
}

// Application Shell
class App {
private:
    std::string stylesheet_path_{""};
    Component root_{};

public:
    App& stylesheet(const std::string& css_path) {
        stylesheet_path_ = css_path;
        return *this;
    }

    App& render(Component root) {
        root_ = std::move(root);
        return *this;
    }

    const Component& root() const {
        return root_;
    }

    const std::string& stylesheet_path() const {
        return stylesheet_path_;
    }
};

} // namespace pingdart::ui

// include/pingdart/ui/components.h
// PingDartUI Core Component Library Factories (PKG-135)
#pragma once

#include <pingdart/ui/component.h>
#include <pingdart/ui/router.h>
#include <string>
#include <vector>
#include <initializer_list>
#include <functional>

namespace pingdart::ui {

// 1. Basic Components
inline Component Text(const std::string& content) {
    return Component("span", content);
}

inline Component Text(std::function<std::string()> reactive_content) {
    return Component("span", std::move(reactive_content));
}

inline Component Heading(const std::string& content, int level = 1) {
    std::string tag = "h" + std::to_string(std::clamp(level, 1, 6));
    auto comp = Component(tag, content);
    if (level == 1) comp.fontSize(32).weight(FontWeight::Bold);
    else if (level == 2) comp.fontSize(24).weight(FontWeight::Bold);
    else if (level == 3) comp.fontSize(20).weight(FontWeight::SemiBold);
    else comp.fontSize(18).weight(FontWeight::Medium);
    return comp;
}

inline Component Image(const std::string& src, const std::string& alt = "") {
    return Component("img").src(src).alt(alt);
}

inline Component Icon(const std::string& name) {
    return Component("i").attr("data-icon", name).className("icon-" + name);
}

inline Component Divider() {
    return Component("hr")
        .width("100%")
        .height(1)
        .background("#E2E8F0")
        .border(0, "transparent");
}

inline Component Spacer(float size = 16.0f) {
    return Component("div").width(size).height(size);
}

// 2. Layout Components
inline Component Container(std::initializer_list<Component> children) {
    return Component("div", children)
        .flexDirection(FlexDirection::Column)
        .width("100%");
}

inline Component Container(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Column)
        .width("100%");
}

inline Component Row(std::initializer_list<Component> children) {
    return Component("div", children)
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center);
}

inline Component Row(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center);
}

inline Component Column(std::initializer_list<Component> children) {
    return Component("div", children)
        .flexDirection(FlexDirection::Column);
}

inline Component Column(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Column);
}

// 3. Buttons & Interaction
inline Component Button(const std::string& label) {
    return Component("button", label)
        .padding(10, 18)
        .borderRadius(8)
        .fontSize(15)
        .weight(FontWeight::Medium)
        .background("#F1F5F9")
        .color("#1E293B");
}

inline Component Link(const std::string& label, const std::string& href) {
    return Component("a", label)
        .href(href)
        .color("#2563EB")
        .onClick([href]() {
            Router::push(href);
        });
}

// 4. Data Display
inline Component Card(std::initializer_list<Component> children) {
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

inline Component Badge(const std::string& label) {
    return Component("span", label)
        .padding(4, 10)
        .borderRadius(9999)
        .fontSize(12)
        .weight(FontWeight::Medium)
        .background("#EFF6FF")
        .color("#2563EB");
}

// 5. Forms
inline Component Input(const std::string& placeholder = "") {
    return Component("input")
        .placeholder(placeholder)
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
}

// 6. Feedback
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

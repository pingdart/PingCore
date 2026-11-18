// include/pingdart/ui/renderer.h
// PingDartUI Pluggable Multi-Renderer Adapters: Web SSR, Qt Quick, & Native RHI (PKG-135)
#pragma once

#include <pingdart/ui/component.h>
#include <string>
#include <sstream>
#include <memory>
#include <algorithm>

namespace pingdart::ui {

class IRenderAdapter {
public:
    virtual ~IRenderAdapter() = default;
    virtual std::string name() const = 0;
    virtual std::string render_to_string(const Component& root) = 0;
};

// 1. Web SSR & HTML5 Renderer
class WebSsrAdapter : public IRenderAdapter {
public:
    std::string name() const override {
        return "WebSSR";
    }

    std::string render_to_string(const Component& root) override {
        std::ostringstream ss;
        render_node(root, ss);
        return ss.str();
    }

private:
    void render_node(const Component& comp, std::ostringstream& ss) {
        ss << "<" << comp.type();

        // Style attribute
        std::string css = comp.computed_style().to_css();
        if (!css.empty()) {
            ss << " style=\"" << css << "\"";
        }

        // Class names
        if (!comp.computed_style().class_names.empty()) {
            ss << " class=\"";
            for (size_t i = 0; i < comp.computed_style().class_names.size(); ++i) {
                if (i > 0) ss << " ";
                ss << comp.computed_style().class_names[i];
            }
            ss << "\"";
        }

        // Attributes
        for (const auto& [k, v] : comp.attributes()) {
            ss << " " << k << "=\"" << v << "\"";
        }

        if (comp.type() == "img" || comp.type() == "hr" || comp.type() == "input") {
            ss << " />";
            return;
        }

        ss << ">";

        // Text Content
        std::string txt = comp.text();
        if (!txt.empty()) {
            ss << txt;
        }

        // Children
        for (const auto& child : comp.children()) {
            render_node(child, ss);
        }

        ss << "</" << comp.type() << ">";
    }
};

// 2. Qt 6 / Qt Quick SceneGraph Adapter (Pluggable)
class QtQuickAdapter : public IRenderAdapter {
public:
    std::string name() const override {
        return "QtQuick";
    }

    std::string render_to_string(const Component& root) override {
        std::ostringstream ss;
        ss << "// PingDartUI Generated Qt Quick SceneGraph\n";
        ss << "import QtQuick 2.15\n";
        ss << "import QtQuick.Controls 2.15\n\n";
        render_qml_item(root, ss, 0);
        return ss.str();
    }

private:
    void render_qml_item(const Component& comp, std::ostringstream& ss, int indent) {
        std::string ind(static_cast<size_t>(std::max(0, indent * 4)), ' ');
        std::string qml_type = "Item";

        if (comp.type() == "span" || comp.type() == "h1" || comp.type() == "h2" || comp.type() == "h3") {
            qml_type = "Text";
        } else if (comp.type() == "button") {
            qml_type = "Button";
        } else if (comp.type() == "div" && comp.computed_style().flex_direction == FlexDirection::Row) {
            qml_type = "Row";
        } else if (comp.type() == "div") {
            qml_type = "Column";
        }

        ss << ind << qml_type << " {\n";

        // Properties
        if (!comp.text().empty()) {
            ss << ind << "    text: \"" << comp.text() << "\"\n";
        }
        if (comp.computed_style().border_radius > 0) {
            ss << ind << "    // radius: " << comp.computed_style().border_radius << "\n";
        }
        if (comp.computed_style().gap > 0) {
            ss << ind << "    spacing: " << comp.computed_style().gap << "\n";
        }

        // Children
        for (const auto& child : comp.children()) {
            render_qml_item(child, ss, indent + 1);
        }

        ss << ind << "}\n";
    }
};

// 3. PingCore Direct Native RHI Adapter
class NativeRhiAdapter : public IRenderAdapter {
public:
    std::string name() const override {
        return "NativeRHI";
    }

    std::string render_to_string(const Component& root) override {
        std::ostringstream ss;
        ss << "[NativeRHI DisplayList: " << root.type() << " tree]";
        return ss.str();
    }
};

} // namespace pingdart::ui

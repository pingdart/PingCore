// include/pingdart/ui/renderer.h
// PingDartUI Pluggable Multi-Renderer Adapters: Web SSR, Qt Quick, & Native RHI (PKG-137)
#pragma once

#include <pingdart/ui/component.h>
#include <pingdart/ui/css_parser.h>
#include <pingdart/ui/theme.h>
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

// 1. Web SSR & HTML5 Renderer with Context-Aware Sanitization
class WebSsrAdapter : public IRenderAdapter {
public:
    std::string name() const override {
        return "WebSSR";
    }

    std::string render_to_string(const Component& root) override {
        std::ostringstream ss;
        render_node(root, ss, true);
        return ss.str();
    }

private:
    void render_node(const Component& comp, std::ostringstream& ss, bool is_root = false) {
        ss << "<" << comp.type();

        // Style attribute with CSS value sanitizer
        std::string css = comp.computed_style().to_css();
        if (!css.empty()) {
            ss << " style=\"" << escape_html(sanitize_css_value(css)) << "\"";
        }

        // Class names
        if (!comp.computed_style().class_names.empty()) {
            ss << " class=\"";
            for (size_t i = 0; i < comp.computed_style().class_names.size(); ++i) {
                if (i > 0) ss << " ";
                ss << escape_html(comp.computed_style().class_names[i]);
            }
            ss << "\"";
        }

        // HTML attributes with URL and entity escaping
        for (const auto& [k, v] : comp.attributes()) {
            if (k == "href" || k == "src") {
                ss << " " << escape_html(k) << "=\"" << escape_html(sanitize_url(v)) << "\"";
            } else {
                ss << " " << escape_html(k) << "=\"" << escape_html(v) << "\"";
            }
        }

        // Automatic dark mode indicator attribute on root element if active
        if (is_root && ThemeEngine::instance().is_dark_mode()) {
            if (comp.attributes().find("data-theme") == comp.attributes().end()) {
                ss << " data-theme=\"dark\"";
            }
        }

        // Void elements in HTML5
        if (comp.type() == "img" || comp.type() == "hr" || comp.type() == "input") {
            ss << " />";
            return;
        }

        ss << ">";

        // Text Content (Neutralized for XSS safety)
        std::string txt = comp.text();
        if (!txt.empty()) {
            ss << escape_html(txt);
        }

        // Children
        for (const auto& child : comp.children()) {
            render_node(child, ss, false);
        }

        ss << "</" << comp.type() << ">";
    }
};

// 2. Qt 6.8 LTS Quick SceneGraph Adapter with Functional Properties and Bindings
class QtQuickAdapter : public IRenderAdapter {
public:
    std::string name() const override {
        return "QtQuick";
    }

    std::string render_to_string(const Component& root) override {
        std::ostringstream ss;
        ss << "// PingDartUI Generated Qt Quick SceneGraph (Qt 6.8 LTS)\n";
        ss << "import QtQuick\n";
        ss << "import QtQuick.Controls\n";
        ss << "import QtQuick.Layouts\n\n";
        render_qml_item(root, ss, 0);
        return ss.str();
    }

private:
    void render_qml_item(const Component& comp, std::ostringstream& ss, int indent) {
        std::string ind(static_cast<size_t>(std::max(0, indent * 4)), ' ');
        std::string qml_type = "Item";

        if (comp.type() == "span" || comp.type() == "p" || comp.type() == "h1" || 
            comp.type() == "h2" || comp.type() == "h3" || comp.type() == "h4") {
            qml_type = "Text";
        } else if (comp.type() == "button") {
            qml_type = "Button";
        } else if (comp.type() == "input") {
            qml_type = "TextField";
        } else if (comp.type() == "textarea") {
            qml_type = "TextArea";
        } else if (comp.type() == "img") {
            qml_type = "Image";
        } else if (comp.type() == "hr") {
            qml_type = "MenuSeparator";
        } else if (comp.type() == "select") {
            qml_type = "ComboBox";
        } else if (comp.type() == "div") {
            if (comp.computed_style().is_grid) {
                qml_type = "Grid";
            } else if (comp.computed_style().flex_direction == FlexDirection::Row) {
                qml_type = "Row";
            } else {
                qml_type = "Column";
            }
        }

        ss << ind << qml_type << " {\n";

        // Properties and functional bindings
        if (!comp.id().empty()) {
            ss << ind << "    id: " << escape_qml_string(comp.id()) << "\n";
        }
        if (!comp.text().empty()) {
            ss << ind << "    text: \"" << escape_qml_string(comp.text()) << "\"\n";
        }
        if (comp.computed_style().border_radius > 0) {
            ss << ind << "    radius: " << comp.computed_style().border_radius << "\n";
        }
        if (comp.computed_style().gap > 0) {
            ss << ind << "    spacing: " << comp.computed_style().gap << "\n";
        }
        if (!comp.computed_style().background_color.empty() && (qml_type == "Rectangle" || qml_type == "Item")) {
            ss << ind << "    color: \"" << escape_qml_string(comp.computed_style().background_color) << "\"\n";
        }

        // Functional layout dimensions
        if (comp.computed_style().width == "100%") {
            ss << ind << "    width: parent.width\n";
        } else if (!comp.computed_style().width.empty()) {
            std::string w = comp.computed_style().width;
            if (w.ends_with("px")) w = w.substr(0, w.size() - 2);
            ss << ind << "    width: " << w << "\n";
        }

        if (comp.computed_style().height == "100%") {
            ss << ind << "    height: parent.height\n";
        } else if (!comp.computed_style().height.empty()) {
            std::string h = comp.computed_style().height;
            if (h.ends_with("px")) h = h.substr(0, h.size() - 2);
            ss << ind << "    height: " << h << "\n";
        }

        // Typography styling for Text nodes
        if (qml_type == "Text") {
            if (comp.computed_style().font_size > 0) {
                ss << ind << "    font.pixelSize: " << comp.computed_style().font_size << "\n";
            }
            if (comp.computed_style().font_weight >= FontWeight::SemiBold) {
                ss << ind << "    font.bold: true\n";
            }
            if (!comp.computed_style().text_color.empty()) {
                ss << ind << "    color: \"" << escape_qml_string(comp.computed_style().text_color) << "\"\n";
            }
        }

        // Image source binding
        if (qml_type == "Image") {
            auto it = comp.attributes().find("src");
            if (it != comp.attributes().end()) {
                ss << ind << "    source: \"" << escape_qml_string(sanitize_url(it->second)) << "\"\n";
            }
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
        ss << "[NativeRHI DisplayList: " << root.type() << " bounds=(" 
           << root.computed_style().layout_bounds.x << ","
           << root.computed_style().layout_bounds.y << ","
           << root.computed_style().layout_bounds.width << ","
           << root.computed_style().layout_bounds.height << ")]";
        return ss.str();
    }
};

} // namespace pingdart::ui

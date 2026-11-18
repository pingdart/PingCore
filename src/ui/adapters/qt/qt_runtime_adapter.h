// src/ui/adapters/qt/qt_runtime_adapter.h
// PingDartUI Qt Quick SceneGraph Runtime Adapter (PKG-138)
#pragma once

#include <pingdart/ui/runtime/component_instance.h>
#include <string>
#include <sstream>
#include <memory>
#include <vector>

namespace pingdart::ui {

class QtRuntimeAdapter {
public:
    static std::string sync_instance_to_qml(const std::shared_ptr<ComponentInstance>& instance) {
        if (!instance) return "";
        std::ostringstream ss;
        render_instance_qml(instance, ss, 0);
        return ss.str();
    }

private:
    static void render_instance_qml(const std::shared_ptr<ComponentInstance>& inst, std::ostringstream& ss, int indent) {
        std::string ind(static_cast<size_t>(indent * 2), ' ');

        std::string qml_type = "Item";
        const auto& t = inst->type();
        if (t == "div" || t == "Card" || t == "Container") qml_type = "Rectangle";
        else if (t == "button" || t == "Button") qml_type = "Button";
        else if (t == "span" || t == "p" || t == "Heading" || t == "Text") qml_type = "Text";
        else if (t == "Row") qml_type = "Row";
        else if (t == "Column") qml_type = "Column";
        else if (t == "input" || t == "Input") qml_type = "TextField";

        ss << ind << qml_type << " {\n";
        ss << ind << "  id: item_" << escape_id(inst->key()) << "\n";

        // Layout properties
        const auto& s = inst->style();
        if (s.layout_bounds.width > 0) {
            ss << ind << "  width: " << s.layout_bounds.width << "\n";
        } else if (!s.width.empty() && s.width != "100%") {
            ss << ind << "  width: " << s.width << "\n";
        }

        if (s.layout_bounds.height > 0) {
            ss << ind << "  height: " << s.layout_bounds.height << "\n";
        } else if (!s.height.empty() && s.height != "100%") {
            ss << ind << "  height: " << s.height << "\n";
        }

        if (qml_type == "Rectangle") {
            if (!s.background_color.empty()) ss << ind << "  color: \"" << s.background_color << "\"\n";
            if (s.border_radius > 0) ss << ind << "  radius: " << s.border_radius << "\n";
        } else if (qml_type == "Text") {
            if (!inst->text().empty()) ss << ind << "  text: \"" << escape_qml_str(inst->text()) << "\"\n";
            if (s.font_size > 0) ss << ind << "  font.pixelSize: " << s.font_size << "\n";
            if (!s.text_color.empty()) ss << ind << "  color: \"" << s.text_color << "\"\n";
        } else if (qml_type == "Button") {
            if (!inst->text().empty()) ss << ind << "  text: \"" << escape_qml_str(inst->text()) << "\"\n";
            ss << ind << "  onClicked: { /* PingDartUI Signal Bridge */ }\n";
        } else if (qml_type == "Row" || qml_type == "Column") {
            if (s.gap > 0) ss << ind << "  spacing: " << s.gap << "\n";
        }

        // Children
        for (const auto& child : inst->children()) {
            if (child) render_instance_qml(child, ss, indent + 1);
        }

        ss << ind << "}\n";
    }

    static std::string escape_id(const std::string& id) {
        std::string res;
        for (char c : id) {
            if (isalnum(c) || c == '_') res += c;
            else res += '_';
        }
        return res.empty() ? "node" : res;
    }

    static std::string escape_qml_str(const std::string& str) {
        std::string res;
        for (char c : str) {
            if (c == '"') res += "\\\"";
            else if (c == '\\') res += "\\\\";
            else if (c == '\n') res += "\\n";
            else res += c;
        }
        return res;
    }
};

} // namespace pingdart::ui

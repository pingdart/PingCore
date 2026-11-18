// src/ui/adapters/web/web_runtime_adapter.h
// PingDartUI Web SSR & Client Hydration Adapter (PKG-138)
#pragma once

#include <pingdart/ui/runtime/component_instance.h>
#include <string>
#include <sstream>
#include <memory>

namespace pingdart::ui {

class WebRuntimeAdapter {
public:
    static std::string render_instance_html(const std::shared_ptr<ComponentInstance>& instance) {
        if (!instance) return "";
        std::ostringstream ss;
        render_node(instance, ss);
        return ss.str();
    }

private:
    static void render_node(const std::shared_ptr<ComponentInstance>& inst, std::ostringstream& ss) {
        ss << "<" << inst->type();
        ss << " data-key=\"" << inst->key() << "\"";

        // Attributes
        for (const auto& [k, v] : inst->attributes()) {
            ss << " " << k << "=\"" << v << "\"";
        }

        std::string css = inst->style().to_css();
        if (!css.empty()) {
            ss << " style=\"" << css << "\"";
        }

        if (inst->type() == "input" || inst->type() == "img" || inst->type() == "hr") {
            ss << " />";
            return;
        }

        ss << ">";

        if (!inst->text().empty()) {
            ss << inst->text();
        }

        for (const auto& child : inst->children()) {
            if (child) render_node(child, ss);
        }

        ss << "</" << inst->type() << ">";
    }
};

} // namespace pingdart::ui

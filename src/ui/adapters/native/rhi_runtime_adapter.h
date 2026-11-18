// src/ui/adapters/native/rhi_runtime_adapter.h
// PingDartUI Native RHI Display List Adapter (PKG-138)
#pragma once

#include <pingdart/ui/runtime/component_instance.h>
#include <pingdart/pal/input_events.h>
#include <memory>
#include <vector>

namespace pingdart::ui {

struct RhiDisplayNode {
    std::string key;
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
    std::string color;
    std::string text;
    std::vector<RhiDisplayNode> children;
};

class RhiRuntimeAdapter {
public:
    static RhiDisplayNode build_display_list(const std::shared_ptr<ComponentInstance>& instance) {
        RhiDisplayNode node;
        if (!instance) return node;

        node.key = instance->key();
        const auto& s = instance->style();
        node.x = s.layout_bounds.x;
        node.y = s.layout_bounds.y;
        node.width = s.layout_bounds.width;
        node.height = s.layout_bounds.height;
        node.color = s.background_color;
        node.text = instance->text();

        for (const auto& child : instance->children()) {
            if (child) {
                node.children.push_back(build_display_list(child));
            }
        }
        return node;
    }

    static std::shared_ptr<ComponentInstance> hit_test(
        const std::shared_ptr<ComponentInstance>& root, float x, float y) {
        if (!root) return nullptr;

        const auto& b = root->style().layout_bounds;
        if (x < b.x || x > b.x + b.width || y < b.y || y > b.y + b.height) {
            return nullptr;
        }

        // Search children back to front
        for (auto it = root->children().rbegin(); it != root->children().rend(); ++it) {
            auto hit = hit_test(*it, x, y);
            if (hit) return hit;
        }

        return root;
    }
};

} // namespace pingdart::ui

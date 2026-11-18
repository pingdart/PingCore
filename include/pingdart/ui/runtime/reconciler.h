// include/pingdart/ui/runtime/reconciler.h
// PingDartUI Keyed Diffing & Reconciliation Engine (PKG-138)
#pragma once

#include <pingdart/ui/component.h>
#include <pingdart/ui/runtime/component_instance.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <memory>

namespace pingdart::ui {

class Reconciler {
public:
    static std::shared_ptr<ComponentInstance> mount(
        const Component& vnode,
        std::shared_ptr<ComponentInstance> parent = nullptr,
        int depth = 0,
        const std::string& fallback_key = "0") {

        std::string key = !vnode.key().empty() ? vnode.key() : fallback_key;
        auto instance = std::make_shared<ComponentInstance>(key, vnode.type());
        instance->set_parent(parent);
        instance->set_depth(depth);
        instance->set_text(vnode.text());
        instance->set_style(vnode.style());
        instance->set_attributes(vnode.attributes());

        sync_listeners(instance, vnode);

        const auto& children = vnode.children();
        for (size_t i = 0; i < children.size(); ++i) {
            std::string child_key = !children[i].key().empty() 
                ? children[i].key() 
                : (key + "." + std::to_string(i));
            auto child_inst = mount(children[i], instance, depth + 1, child_key);
            instance->children().push_back(child_inst);
        }

        instance->mount();
        return instance;
    }

    static std::shared_ptr<ComponentInstance> reconcile(
        std::shared_ptr<ComponentInstance> current,
        const Component& next_vnode,
        int depth = 0,
        const std::string& fallback_key = "0") {

        if (!current) {
            return mount(next_vnode, nullptr, depth, fallback_key);
        }

        // 1. Type mismatch -> destroy old, mount new
        if (current->type() != next_vnode.type()) {
            auto parent = current->parent();
            current->unmount();
            return mount(next_vnode, parent, depth, fallback_key);
        }

        // 2. Types match: update current instance in-place
        current->set_depth(depth);
        current->set_text(next_vnode.text());
        current->set_style(next_vnode.style());
        current->set_attributes(next_vnode.attributes());

        sync_listeners(current, next_vnode);

        // 3. Keyed Children Reconciliation
        std::unordered_map<std::string, std::shared_ptr<ComponentInstance>> key_map;
        std::vector<std::shared_ptr<ComponentInstance>> unkeyed_pool;

        for (const auto& old_child : current->children()) {
            if (old_child) {
                if (!old_child->key().empty() && old_child->key().find('.') == std::string::npos) {
                    key_map[old_child->key()] = old_child;
                } else {
                    unkeyed_pool.push_back(old_child);
                }
            }
        }

        std::vector<std::shared_ptr<ComponentInstance>> next_children;
        std::unordered_set<ComponentInstance*> retained_set;

        const auto& new_children = next_vnode.children();
        size_t unkeyed_idx = 0;

        for (size_t i = 0; i < new_children.size(); ++i) {
            const auto& child_vnode = new_children[i];
            std::string explicit_key = child_vnode.key();

            std::shared_ptr<ComponentInstance> matched = nullptr;

            // Match by key if present
            if (!explicit_key.empty()) {
                auto it = key_map.find(explicit_key);
                if (it != key_map.end()) {
                    matched = it->second;
                }
            } else if (unkeyed_idx < unkeyed_pool.size()) {
                // Match by position
                matched = unkeyed_pool[unkeyed_idx++];
            }

            if (matched) {
                // Reconcile existing matched child
                auto reconciled_child = reconcile(matched, child_vnode, depth + 1, 
                    explicit_key.empty() ? (current->key() + "." + std::to_string(i)) : explicit_key);
                reconciled_child->set_parent(current);
                next_children.push_back(reconciled_child);
                retained_set.insert(reconciled_child.get());
            } else {
                // Mount new child
                std::string child_key = !explicit_key.empty() 
                    ? explicit_key 
                    : (current->key() + "." + std::to_string(i));
                auto new_child = mount(child_vnode, current, depth + 1, child_key);
                next_children.push_back(new_child);
                retained_set.insert(new_child.get());
            }
        }

        // 4. Deterministic cleanup of unmounted children
        for (auto& old_child : current->children()) {
            if (old_child && retained_set.find(old_child.get()) == retained_set.end()) {
                old_child->unmount();
            }
        }

        current->children() = std::move(next_children);
        return current;
    }

private:
    static void sync_listeners(const std::shared_ptr<ComponentInstance>& inst, const Component& vnode) {
        inst->clear_listeners();

        // Transfer click handler
        inst->add_listener(EventType::Click, [vnode](Event& /*e*/) {
            vnode.emit_click();
        });

        // Transfer change handler
        inst->add_listener(EventType::Change, [vnode](Event& /*e*/) {
            vnode.emit_change("");
        });

        // Gestures
        inst->set_on_tap([vnode](const TapEvent& /*e*/) {
            vnode.emit_click();
        });

        inst->set_on_long_press([](const LongPressEvent& /*e*/) {});
        inst->set_on_swipe([](const SwipeEvent& /*e*/) {});
        inst->set_on_pan([](const PanEvent& /*e*/) {});
        inst->set_on_pinch([](const PinchEvent& /*e*/) {});
    }
};

} // namespace pingdart::ui

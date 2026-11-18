// include/pingdart/ui/runtime/component_instance.h
// PingDartUI Persistent Component Instance & State Model (PKG-138)
#pragma once

#include <pingdart/ui/style.h>
#include <pingdart/ui/events/event.h>
#include <pingdart/ui/gestures/tap.h>
#include <pingdart/ui/gestures/swipe.h>
#include <pingdart/ui/gestures/pinch.h>
#include <pingdart/ui/gestures/drag.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <any>

namespace pingdart::ui {

class Component;

class ComponentInstance : public std::enable_shared_from_this<ComponentInstance> {
private:
    std::string key_{""};
    std::string type_{"div"};
    std::string text_{""};
    ComputedStyle style_{};
    std::unordered_map<std::string, std::string> attributes_{};

    std::weak_ptr<ComponentInstance> parent_{};
    std::vector<std::shared_ptr<ComponentInstance>> children_{};
    int depth_{0};

    bool is_dirty_{false};
    bool is_mounted_{false};

    // State storage slots for hooks (preserved across re-renders)
    std::unordered_map<std::string, std::any> state_slots_{};

    // Sequential hook slots for useCallback / useTheme / useMemo / useEffect.
    // Each hook call during a render consumes the next slot in order.
    std::vector<std::any> hook_slots_{};
    std::size_t hook_cursor_{0};

    // Lifecycle callbacks
    std::vector<std::function<void()>> mount_callbacks_{};
    std::vector<std::function<void()>> update_callbacks_{};
    std::vector<std::function<void()>> cleanup_callbacks_{};

    // Event listeners: Phase -> EventType -> List of handlers
    std::unordered_map<EventType, std::vector<std::function<void(Event&)>>> listeners_{};
    std::unordered_map<EventType, std::vector<std::function<void(Event&)>>> capture_listeners_{};

    // Gesture handlers
    std::function<void(const TapEvent&)> on_tap_{nullptr};
    std::function<void(const LongPressEvent&)> on_long_press_{nullptr};
    std::function<void(const SwipeEvent&)> on_swipe_{nullptr};
    std::function<void(const PanEvent&)> on_pan_{nullptr};
    std::function<void(const PinchEvent&)> on_pinch_{nullptr};

public:
    ComponentInstance(std::string key, std::string type)
        : key_(std::move(key)), type_(std::move(type)) {}

    ~ComponentInstance() {
        if (is_mounted_) {
            unmount();
        }
    }

    const std::string& key() const noexcept { return key_; }
    const std::string& type() const noexcept { return type_; }
    const std::string& text() const noexcept { return text_; }
    void set_text(std::string t) { text_ = std::move(t); }

    const ComputedStyle& style() const noexcept { return style_; }
    ComputedStyle& style() noexcept { return style_; }
    void set_style(ComputedStyle s) { style_ = std::move(s); }

    const std::unordered_map<std::string, std::string>& attributes() const noexcept { return attributes_; }
    void set_attribute(const std::string& k, const std::string& v) { attributes_[k] = v; }
    void set_attributes(std::unordered_map<std::string, std::string> attrs) { attributes_ = std::move(attrs); }

    std::shared_ptr<ComponentInstance> parent() const noexcept { return parent_.lock(); }
    void set_parent(std::shared_ptr<ComponentInstance> p) noexcept { parent_ = p; }

    const std::vector<std::shared_ptr<ComponentInstance>>& children() const noexcept { return children_; }
    std::vector<std::shared_ptr<ComponentInstance>>& children() noexcept { return children_; }

    int depth() const noexcept { return depth_; }
    void set_depth(int d) noexcept { depth_ = d; }

    bool is_dirty() const noexcept { return is_dirty_; }
    void set_dirty(bool d) noexcept { is_dirty_ = d; }

    bool is_mounted() const noexcept { return is_mounted_; }

    // ----------------------------------------------------------------
    // Sequential hook-slot API (useCallback, useTheme, useMemo, …)
    // ----------------------------------------------------------------
    // Call begin_render() at the start of each component render to
    // reset the cursor. Each next_hook<T>() call returns a stable
    // reference to the T stored in the growing slot vector, allocating
    // a default-constructed T the first time the slot is encountered.

    void begin_render() noexcept {
        hook_cursor_ = 0;
    }

    template <typename T>
    T& next_hook() {
        if (hook_cursor_ >= hook_slots_.size()) {
            hook_slots_.emplace_back(T{});
        }
        auto& slot = hook_slots_[hook_cursor_++];
        // If the stored any holds the wrong type (e.g. first use after
        // a type change during hot-reload) replace it with a fresh T.
        if (slot.type() != typeid(T)) {
            slot = T{};
        }
        return std::any_cast<T&>(slot);
    }

    // ----------------------------------------------------------------
    // Hook State Storage (Stable Across Renders) — keyed variant
    // ----------------------------------------------------------------
    template <typename T>
    std::shared_ptr<T> get_or_create_state(const std::string& slot_id, T initial_val) {
        auto it = state_slots_.find(slot_id);
        if (it != state_slots_.end()) {
            try {
                return std::any_cast<std::shared_ptr<T>>(it->second);
            } catch (...) {}
        }
        auto ptr = std::make_shared<T>(std::move(initial_val));
        state_slots_[slot_id] = ptr;
        return ptr;
    }

    // Lifecycle
    void add_mount_callback(std::function<void()> cb) {
        mount_callbacks_.push_back(std::move(cb));
    }

    void add_update_callback(std::function<void()> cb) {
        update_callbacks_.push_back(std::move(cb));
    }

    void add_cleanup_callback(std::function<void()> cb) {
        cleanup_callbacks_.push_back(std::move(cb));
    }

    void mount() {
        if (is_mounted_) return;
        is_mounted_ = true;
        for (const auto& cb : mount_callbacks_) {
            if (cb) cb();
        }
        for (const auto& child : children_) {
            if (child) child->mount();
        }
    }

    void update_lifecycle() {
        begin_render(); // reset hook cursor before each re-render
        for (const auto& cb : update_callbacks_) {
            if (cb) cb();
        }
    }

    void unmount() {
        if (!is_mounted_) return;
        is_mounted_ = false;

        // Run all cleanup handlers (e.g. useEffect cleanups)
        for (const auto& cb : cleanup_callbacks_) {
            if (cb) {
                try { cb(); } catch (...) {}
            }
        }
        cleanup_callbacks_.clear();

        // Unmount children recursively
        for (const auto& child : children_) {
            if (child) child->unmount();
        }
        children_.clear();
    }

    // Event Registration
    void add_listener(EventType type, std::function<void(Event&)> cb, bool capture = false) {
        if (capture) {
            capture_listeners_[type].push_back(std::move(cb));
        } else {
            listeners_[type].push_back(std::move(cb));
        }
    }

    void clear_listeners() {
        listeners_.clear();
        capture_listeners_.clear();
    }

    void invoke_listeners(Event& event, bool capture) {
        const auto& map = capture ? capture_listeners_ : listeners_;
        auto it = map.find(event.type());
        if (it != map.end()) {
            for (const auto& handler : it->second) {
                if (handler) {
                    handler(event);
                    if (event.is_immediate_propagation_stopped()) {
                        break;
                    }
                }
            }
        }
    }

    // Gestures
    void set_on_tap(std::function<void(const TapEvent&)> h) { on_tap_ = std::move(h); }
    void set_on_long_press(std::function<void(const LongPressEvent&)> h) { on_long_press_ = std::move(h); }
    void set_on_swipe(std::function<void(const SwipeEvent&)> h) { on_swipe_ = std::move(h); }
    void set_on_pan(std::function<void(const PanEvent&)> h) { on_pan_ = std::move(h); }
    void set_on_pinch(std::function<void(const PinchEvent&)> h) { on_pinch_ = std::move(h); }

    const std::function<void(const TapEvent&)>& on_tap() const noexcept { return on_tap_; }
    const std::function<void(const LongPressEvent&)>& on_long_press() const noexcept { return on_long_press_; }
    const std::function<void(const SwipeEvent&)>& on_swipe() const noexcept { return on_swipe_; }
    const std::function<void(const PanEvent&)>& on_pan() const noexcept { return on_pan_; }
    const std::function<void(const PinchEvent&)>& on_pinch() const noexcept { return on_pinch_; }
};

} // namespace pingdart::ui

// include/pingdart/ui/events/event.h
// PingDartUI Unified Cross-Platform Event Model (PKG-138)
#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <memory>
#include <unordered_map>

namespace pingdart::ui {

enum class EventPhase {
    None,
    Capturing,
    AtTarget,
    Bubbling
};

enum class EventType {
    // Pointer / Mouse / Touch
    Click,
    DoubleClick,
    PointerDown,
    PointerUp,
    PointerMove,
    PointerEnter,
    PointerLeave,
    PointerCancel,
    
    // Keyboard
    KeyDown,
    KeyUp,
    KeyPress,
    
    // Focus
    Focus,
    Blur,
    FocusIn,
    FocusOut,
    
    // Form
    Change,
    Input,
    Submit,
    Reset,
    
    // Scroll & Layout
    Scroll,
    ScrollEnd,
    Resize,
    
    // Gestures
    Tap,
    DoubleTap,
    LongPress,
    Swipe,
    Pan,
    Pinch,
    DragStart,
    DragOver,
    Drop,
    
    // Component Lifecycle
    Mount,
    Update,
    Unmount,
    
    // Custom
    Custom
};

inline std::string event_type_to_string(EventType type) {
    switch (type) {
        case EventType::Click: return "click";
        case EventType::DoubleClick: return "dblclick";
        case EventType::PointerDown: return "pointerdown";
        case EventType::PointerUp: return "pointerup";
        case EventType::PointerMove: return "pointermove";
        case EventType::PointerEnter: return "pointerenter";
        case EventType::PointerLeave: return "pointerleave";
        case EventType::PointerCancel: return "pointercancel";
        case EventType::KeyDown: return "keydown";
        case EventType::KeyUp: return "keyup";
        case EventType::KeyPress: return "keypress";
        case EventType::Focus: return "focus";
        case EventType::Blur: return "blur";
        case EventType::FocusIn: return "focusin";
        case EventType::FocusOut: return "focusout";
        case EventType::Change: return "change";
        case EventType::Input: return "input";
        case EventType::Submit: return "submit";
        case EventType::Reset: return "reset";
        case EventType::Scroll: return "scroll";
        case EventType::ScrollEnd: return "scrollend";
        case EventType::Resize: return "resize";
        case EventType::Tap: return "tap";
        case EventType::DoubleTap: return "doubletap";
        case EventType::LongPress: return "longpress";
        case EventType::Swipe: return "swipe";
        case EventType::Pan: return "pan";
        case EventType::Pinch: return "pinch";
        case EventType::DragStart: return "dragstart";
        case EventType::DragOver: return "dragover";
        case EventType::Drop: return "drop";
        case EventType::Mount: return "mount";
        case EventType::Update: return "update";
        case EventType::Unmount: return "unmount";
        case EventType::Custom: return "custom";
    }
    return "unknown";
}

class ComponentInstance;

class Event {
protected:
    EventType type_{EventType::Custom};
    std::string name_{"custom"};
    std::string target_id_{""};
    std::shared_ptr<ComponentInstance> target_{nullptr};
    std::shared_ptr<ComponentInstance> current_target_{nullptr};
    EventPhase phase_{EventPhase::None};
    uint64_t timestamp_us_{0};
    bool bubbles_{true};
    bool cancelable_{true};
    bool propagation_stopped_{false};
    bool immediate_propagation_stopped_{false};
    bool default_prevented_{false};

public:
    explicit Event(EventType type, bool bubbles = true, bool cancelable = true)
        : type_(type),
          name_(event_type_to_string(type)),
          bubbles_(bubbles),
          cancelable_(cancelable) {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        timestamp_us_ = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
    }

    virtual ~Event() = default;

    EventType type() const noexcept { return type_; }
    const std::string& name() const noexcept { return name_; }
    EventPhase phase() const noexcept { return phase_; }
    void set_phase(EventPhase p) noexcept { phase_ = p; }

    std::shared_ptr<ComponentInstance> target() const noexcept { return target_; }
    void set_target(std::shared_ptr<ComponentInstance> t) noexcept { target_ = t; }

    std::shared_ptr<ComponentInstance> current_target() const noexcept { return current_target_; }
    void set_current_target(std::shared_ptr<ComponentInstance> ct) noexcept { current_target_ = ct; }

    const std::string& target_id() const noexcept { return target_id_; }
    void set_target_id(std::string id) noexcept { target_id_ = std::move(id); }

    uint64_t timestamp_us() const noexcept { return timestamp_us_; }
    bool bubbles() const noexcept { return bubbles_; }
    bool cancelable() const noexcept { return cancelable_; }

    void stopPropagation() noexcept { propagation_stopped_ = true; }
    void stopImmediatePropagation() noexcept { 
        propagation_stopped_ = true;
        immediate_propagation_stopped_ = true;
    }
    void preventDefault() noexcept {
        if (cancelable_) default_prevented_ = true;
    }

    bool is_propagation_stopped() const noexcept { return propagation_stopped_; }
    bool is_immediate_propagation_stopped() const noexcept { return immediate_propagation_stopped_; }
    bool is_default_prevented() const noexcept { return default_prevented_; }
};

} // namespace pingdart::ui

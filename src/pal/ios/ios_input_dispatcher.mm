// src/pal/ios/ios_input_dispatcher.mm
#include <pingdart/pal/ios/ios_platform.h>
#include <pingdart/pal/i_input_dispatcher.h>

namespace pingdart::pal::ios {

void IOSWindow::set_safe_area_insets(const IOSSafeAreaInsets& insets) noexcept {
    safe_area_ = insets;
}

void IOSWindow::update_safe_area_insets(const IOSSafeAreaInsets& insets) {
    if (safe_area_ != insets) {
        safe_area_ = insets;
        if (on_safe_area_changed_) {
            on_safe_area_changed_(insets);
        }
    }
}

void IOSWindow::set_on_safe_area_changed(std::function<void(const IOSSafeAreaInsets&)> callback) noexcept {
    on_safe_area_changed_ = std::move(callback);
}

const IOSSafeAreaInsets& IOSWindow::safe_area_insets() const noexcept {
    return safe_area_;
}

void IOSWindow::handle_ios_touches(TouchPhase phase, const std::vector<TouchPoint>& touches) {
    if (!dispatcher_ || touches.empty()) {
        return;
    }

    EventType target_type = EventType::PointerMove;
    PointerButton btn = PointerButton::Left;

    switch (phase) {
        case TouchPhase::Began:
            target_type = EventType::PointerDown;
            break;
        case TouchPhase::Moved:
            target_type = EventType::PointerMove;
            break;
        case TouchPhase::Ended:
            target_type = EventType::PointerUp;
            break;
        case TouchPhase::Cancelled:
            target_type = EventType::PointerLeave;
            btn = PointerButton::None;
            break;
        case TouchPhase::Stationary:
            return;
    }

    for (const auto& touch : touches) {
        PointerEvent pe{};
        pe.type = target_type;
        pe.pointer_id = touch.touch_id;
        pe.x = touch.x;
        pe.y = touch.y;
        pe.pressure = touch.pressure;
        pe.button = btn;
        pe.buttons_down = (btn != PointerButton::None ? static_cast<uint8_t>(btn) : uint8_t{0});
        pe.timestamp_us = touch.timestamp_us;
        dispatcher_->dispatch_pointer_event(pe);
    }
}

} // namespace pingdart::pal::ios

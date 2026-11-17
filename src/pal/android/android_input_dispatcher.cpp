// src/pal/android/android_input_dispatcher.cpp
#include <pingdart/pal/android/android_platform.h>
#include <pingdart/pal/i_input_dispatcher.h>

namespace pingdart::pal::android {

void AndroidWindow::dispatch_motion_event(
    AndroidMotionEventAction action,
    uint32_t pointer_index,
    const std::vector<TouchPoint>& touches)
{
    if (!dispatcher_ || touches.empty()) {
        return;
    }

    EventType target_type = EventType::PointerMove;
    PointerButton btn = PointerButton::None;

    switch (action) {
        case AndroidMotionEventAction::Down:
        case AndroidMotionEventAction::PointerDown:
            target_type = EventType::PointerDown;
            btn = PointerButton::Left;
            break;
        case AndroidMotionEventAction::Up:
        case AndroidMotionEventAction::PointerUp:
            target_type = EventType::PointerUp;
            btn = PointerButton::Left;
            break;
        case AndroidMotionEventAction::Move:
            target_type = EventType::PointerMove;
            btn = PointerButton::Left;
            break;
        case AndroidMotionEventAction::Cancel:
            target_type = EventType::PointerLeave;
            break;
        default:
            return;
    }

    if (action == AndroidMotionEventAction::Move) {
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
    } else {
        const auto& active_touch = (pointer_index < touches.size()) ? touches[pointer_index] : touches[0];
        PointerEvent pe{};
        pe.type = target_type;
        pe.pointer_id = active_touch.touch_id;
        pe.x = active_touch.x;
        pe.y = active_touch.y;
        pe.pressure = active_touch.pressure;
        pe.button = btn;
        pe.buttons_down = (action == AndroidMotionEventAction::Down || action == AndroidMotionEventAction::PointerDown) ? static_cast<uint8_t>(PointerButton::Left) : uint8_t{0};
        pe.timestamp_us = active_touch.timestamp_us;
        dispatcher_->dispatch_pointer_event(pe);
    }
}

} // namespace pingdart::pal::android

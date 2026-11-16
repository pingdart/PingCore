#include <pingdart/ui/input_dispatcher.h>

namespace pingdart::ui {

UIInputDispatcher::UIInputDispatcher(WidgetTree& widget_tree, FocusManager& focus_manager)
    : widget_tree_(widget_tree), focus_manager_(focus_manager) {}

void UIInputDispatcher::dispatch_pointer_event(const pal::PointerEvent& event) {
    Offset pos{static_cast<float>(event.x), static_cast<float>(event.y)};
    last_pointer_pos_ = pos;

    if (event.type == pal::EventType::PointerDown) {
        HitTestResult hit = widget_tree_.hit_test(pos);
        active_gesture_detector_ = nullptr;
        for (const auto& entry : hit.path()) {
            auto* gd = dynamic_cast<RenderGestureDetector*>(entry.target);
            if (gd) {
                active_gesture_detector_ = gd;
                gd->trigger_tap();
                break;
            }
        }
    } else if (event.type == pal::EventType::PointerUp) {
        active_gesture_detector_ = nullptr;
    }
}

void UIInputDispatcher::dispatch_keyboard_event(const pal::KeyboardEvent& event) {
    if (event.type == pal::EventType::KeyDown) {
        // Tab / Shift+Tab keyboard focus traversal
        if (event.virtual_key == 9 || event.key_code == 0xFF09) {
            bool shift = (event.modifiers & pal::ModifierKey::Shift);
            if (focus_manager_.advance_focus(shift)) {
                return;
            }
        }
    }

    focus_manager_.dispatch_key_event(event);
}

void UIInputDispatcher::dispatch_lifecycle_event(const pal::WindowLifecycleEvent& event) {
    if (event.type == pal::EventType::WindowResized) {
        widget_tree_.flush_layout(Size{static_cast<float>(event.width), static_cast<float>(event.height)}, true);
        (void)widget_tree_.flush_paint();
    }
}

void UIInputDispatcher::dispatch_text_event(const pal::TextInputEvent& event) {
    if (focus_manager_.primary_focus()) {
        focus_manager_.primary_focus()->handle_text(event);
    }
}

} // namespace pingdart::ui

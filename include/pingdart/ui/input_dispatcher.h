#pragma once

#include <pingdart/pal/i_input_dispatcher.h>
#include <pingdart/ui/widget_tree.h>
#include <pingdart/ui/focus_node.h>
#include <pingdart/ui/layout_primitives.h>
#include <memory>

namespace pingdart::ui {

class UIInputDispatcher : public pal::IInputDispatcher {
public:
    UIInputDispatcher(WidgetTree& widget_tree, FocusManager& focus_manager);
    ~UIInputDispatcher() override = default;

    UIInputDispatcher(const UIInputDispatcher&) = delete;
    UIInputDispatcher& operator=(const UIInputDispatcher&) = delete;

    void dispatch_pointer_event(const pal::PointerEvent& event) override;
    void dispatch_keyboard_event(const pal::KeyboardEvent& event) override;
    void dispatch_lifecycle_event(const pal::WindowLifecycleEvent& event) override;
    void dispatch_text_event(const pal::TextInputEvent& event) override;

    [[nodiscard]] FocusManager& focus_manager() noexcept { return focus_manager_; }
    [[nodiscard]] WidgetTree& widget_tree() noexcept { return widget_tree_; }

    [[nodiscard]] RenderGestureDetector* active_gesture_detector() const noexcept {
        return active_gesture_detector_;
    }

private:
    WidgetTree& widget_tree_;
    FocusManager& focus_manager_;
    RenderGestureDetector* active_gesture_detector_{nullptr};
    Offset last_pointer_pos_{0.0f, 0.0f};
};

} // namespace pingdart::ui

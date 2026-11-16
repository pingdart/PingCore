// include/pingdart/pal/i_input_dispatcher.h
#pragma once
#include <pingdart/pal/input_events.h>

namespace pingdart::pal {

class IInputDispatcher {
public:
    virtual ~IInputDispatcher() = default;

    virtual void dispatch_pointer_event(const PointerEvent& event) = 0;
    virtual void dispatch_keyboard_event(const KeyboardEvent& event) = 0;
    virtual void dispatch_lifecycle_event(const WindowLifecycleEvent& event) = 0;

    virtual void dispatch_key_event(const KeyEvent& event) {
        // Default adapter conversion
        KeyboardEvent ke;
        ke.type = (event.action == KeyAction::Down) ? EventType::KeyDown : EventType::KeyUp;
        ke.key_code = event.physical_key;
        ke.virtual_key = event.logical_key;
        ke.modifiers = static_cast<ModifierKey>(event.modifiers);
        ke.is_repeat = (event.action == KeyAction::Repeat);
        ke.timestamp_us = event.timestamp_ns / 1000;
        dispatch_keyboard_event(ke);
    }

    virtual void dispatch_text_event(const TextInputEvent& event) {
        // Default adapter conversion
        (void)event;
    }
};

} // namespace pingdart::pal

// include/pingdart/ui/gestures/gesture.h
// PingDartUI Gesture System Base Types (PKG-138)
#pragma once

#include <pingdart/ui/events/event.h>
#include <pingdart/ui/events/pointer_event.h>
#include <string>
#include <functional>
#include <cmath>

namespace pingdart::ui {

enum class GestureType {
    Tap,
    DoubleTap,
    LongPress,
    Swipe,
    Pan,
    Pinch,
    DragDrop
};

class GestureEvent : public Event {
protected:
    float x_{0.0f};
    float y_{0.0f};

public:
    GestureEvent(EventType type, float x, float y)
        : Event(type, true, true), x_(x), y_(y) {}

    float x() const noexcept { return x_; }
    float y() const noexcept { return y_; }
};

} // namespace pingdart::ui

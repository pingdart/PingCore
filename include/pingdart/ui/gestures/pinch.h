// include/pingdart/ui/gestures/pinch.h
// PingDartUI Multi-Touch Pinch Gesture (PKG-138)
#pragma once

#include <pingdart/ui/gestures/gesture.h>

namespace pingdart::ui {

class PinchEvent : public GestureEvent {
public:
    float scale{1.0f};
    float focal_x{0.0f};
    float focal_y{0.0f};

    PinchEvent(float s, float fx, float fy)
        : GestureEvent(EventType::Pinch, fx, fy),
          scale(s),
          focal_x(fx),
          focal_y(fy) {}
};

} // namespace pingdart::ui

// include/pingdart/ui/gestures/swipe.h
// PingDartUI Swipe Gesture Recognition (PKG-138)
#pragma once

#include <pingdart/ui/gestures/gesture.h>

namespace pingdart::ui {

enum class Direction {
    None,
    Left,
    Right,
    Up,
    Down
};

class SwipeEvent : public GestureEvent {
public:
    Direction direction{Direction::None};
    float velocity_x{0.0f};
    float velocity_y{0.0f};
    float distance{0.0f};

    SwipeEvent(Direction dir, float vel_x, float vel_y, float dist, float x = 0.0f, float y = 0.0f)
        : GestureEvent(EventType::Swipe, x, y),
          direction(dir),
          velocity_x(vel_x),
          velocity_y(vel_y),
          distance(dist) {}
};

} // namespace pingdart::ui

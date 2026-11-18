// include/pingdart/ui/gestures/tap.h
// PingDartUI Tap & Long Press Gestures (PKG-138)
#pragma once

#include <pingdart/ui/gestures/gesture.h>

namespace pingdart::ui {

class TapEvent : public GestureEvent {
private:
    int tap_count_{1};

public:
    TapEvent(float x, float y, int tap_count = 1)
        : GestureEvent(tap_count > 1 ? EventType::DoubleTap : EventType::Tap, x, y),
          tap_count_(tap_count) {}

    int tap_count() const noexcept { return tap_count_; }
};

class LongPressEvent : public GestureEvent {
private:
    uint64_t duration_ms_{500};

public:
    LongPressEvent(float x, float y, uint64_t duration_ms = 500)
        : GestureEvent(EventType::LongPress, x, y),
          duration_ms_(duration_ms) {}

    uint64_t duration_ms() const noexcept { return duration_ms_; }
};

} // namespace pingdart::ui

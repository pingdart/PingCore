// src/ui/gestures/gesture.cpp
// PingDartUI Gesture Recognizer Implementation (PKG-138)
#include <pingdart/ui/gestures/gesture.h>
#include <pingdart/ui/gestures/tap.h>
#include <pingdart/ui/gestures/swipe.h>
#include <pingdart/ui/gestures/pinch.h>
#include <pingdart/ui/gestures/drag.h>

namespace pingdart::ui {

// Gesture recognizer state machine logic
class GestureRecognizer {
private:
    float start_x_{0.0f};
    float start_y_{0.0f};
    uint64_t start_time_us_{0};
    bool is_down_{false};

public:
    void handle_pointer_down(float x, float y, uint64_t time_us) {
        start_x_ = x;
        start_y_ = y;
        start_time_us_ = time_us;
        is_down_ = true;
    }

    SwipeEvent check_swipe(float end_x, float end_y, uint64_t end_time_us) {
        if (!is_down_) return SwipeEvent(Direction::None, 0, 0, 0);
        float dx = end_x - start_x_;
        float dy = end_y - start_y_;
        float dist = std::sqrt(dx * dx + dy * dy);
        float dt_sec = static_cast<float>(end_time_us - start_time_us_) / 1000000.0f;
        if (dt_sec <= 0.001f) dt_sec = 0.001f;

        Direction dir = Direction::None;
        if (std::abs(dx) > std::abs(dy)) {
            dir = (dx > 0) ? Direction::Right : Direction::Left;
        } else {
            dir = (dy > 0) ? Direction::Down : Direction::Up;
        }

        is_down_ = false;
        return SwipeEvent(dir, dx / dt_sec, dy / dt_sec, dist, end_x, end_y);
    }
};

} // namespace pingdart::ui

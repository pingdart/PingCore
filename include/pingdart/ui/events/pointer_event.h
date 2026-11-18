// include/pingdart/ui/events/pointer_event.h
// PingDartUI Pointer, Mouse & Touch Events (PKG-138)
#pragma once

#include <pingdart/ui/events/event.h>

namespace pingdart::ui {

enum class PointerButton {
    None = -1,
    Primary = 0,   // Left
    Middle = 1,    // Middle wheel
    Secondary = 2  // Right
};

class PointerEvent : public Event {
private:
    float client_x_{0.0f};
    float client_y_{0.0f};
    float screen_x_{0.0f};
    float screen_y_{0.0f};
    PointerButton button_{PointerButton::Primary};
    int buttons_{0};
    bool alt_key_{false};
    bool ctrl_key_{false};
    bool shift_key_{false};
    bool meta_key_{false};
    float pressure_{0.5f};
    std::string pointer_type_{"mouse"}; // "mouse", "touch", "pen"

public:
    PointerEvent(EventType type, float x, float y, PointerButton btn = PointerButton::Primary)
        : Event(type, true, true),
          client_x_(x),
          client_y_(y),
          screen_x_(x),
          screen_y_(y),
          button_(btn) {}

    float client_x() const noexcept { return client_x_; }
    float client_y() const noexcept { return client_y_; }
    float screen_x() const noexcept { return screen_x_; }
    float screen_y() const noexcept { return screen_y_; }
    PointerButton button() const noexcept { return button_; }
    int buttons() const noexcept { return buttons_; }
    float pressure() const noexcept { return pressure_; }
    const std::string& pointer_type() const noexcept { return pointer_type_; }

    bool alt_key() const noexcept { return alt_key_; }
    bool ctrl_key() const noexcept { return ctrl_key_; }
    bool shift_key() const noexcept { return shift_key_; }
    bool meta_key() const noexcept { return meta_key_; }

    void set_modifiers(bool alt, bool ctrl, bool shift, bool meta) noexcept {
        alt_key_ = alt;
        ctrl_key_ = ctrl;
        shift_key_ = shift;
        meta_key_ = meta;
    }

    void set_pressure(float p) noexcept { pressure_ = p; }
    void set_pointer_type(std::string pt) noexcept { pointer_type_ = std::move(pt); }
    void set_buttons(int b) noexcept { buttons_ = b; }
};

} // namespace pingdart::ui

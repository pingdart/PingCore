// include/pingdart/ui/events/keyboard_event.h
// PingDartUI Keyboard Event Model (PKG-138)
#pragma once

#include <pingdart/ui/events/event.h>
#include <string>

namespace pingdart::ui {

class KeyboardEvent : public Event {
private:
    std::string key_{""};
    std::string code_{""};
    bool repeat_{false};
    bool alt_key_{false};
    bool ctrl_key_{false};
    bool shift_key_{false};
    bool meta_key_{false};

public:
    KeyboardEvent(EventType type, std::string key, std::string code, bool repeat = false)
        : Event(type, true, true),
          key_(std::move(key)),
          code_(std::move(code)),
          repeat_(repeat) {}

    const std::string& key() const noexcept { return key_; }
    const std::string& code() const noexcept { return code_; }
    bool repeat() const noexcept { return repeat_; }

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
};

} // namespace pingdart::ui

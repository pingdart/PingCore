// include/pingdart/ui/events/focus_manager.h
// PingDartUI Focus Management System (PKG-138)
#pragma once

#include <pingdart/ui/events/event.h>
#include <pingdart/ui/runtime/component_instance.h>
#include <memory>
#include <string>

namespace pingdart::ui {

class FocusEvent : public Event {
private:
    std::string related_target_id_{""};

public:
    explicit FocusEvent(EventType type, std::string related_id = "")
        : Event(type, false, false),
          related_target_id_(std::move(related_id)) {}

    const std::string& related_target_id() const noexcept { return related_target_id_; }
};

class FocusManager {
private:
    std::shared_ptr<ComponentInstance> focused_instance_{nullptr};

public:
    FocusManager() = default;

    static FocusManager& instance() {
        static FocusManager s_instance;
        return s_instance;
    }

    std::shared_ptr<ComponentInstance> focused() const noexcept {
        return focused_instance_;
    }

    void set_focus(std::shared_ptr<ComponentInstance> target) {
        if (focused_instance_ == target) return;

        if (focused_instance_) {
            FocusEvent blur_evt(EventType::Blur, target ? target->key() : "");
            focused_instance_->invoke_listeners(blur_evt, false);
        }

        auto old_target = focused_instance_;
        focused_instance_ = target;

        if (focused_instance_) {
            FocusEvent focus_evt(EventType::Focus, old_target ? old_target->key() : "");
            focused_instance_->invoke_listeners(focus_evt, false);
        }
    }

    void clear_focus() {
        set_focus(nullptr);
    }
};

} // namespace pingdart::ui

#pragma once

#include <pingdart/pal/input_events.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>

namespace pingdart::ui {

class FocusManager;

class FocusNode : public std::enable_shared_from_this<FocusNode> {
public:
    explicit FocusNode(std::string debug_label = "", int32_t tab_order = 0);
    virtual ~FocusNode();

    FocusNode(const FocusNode&) = delete;
    FocusNode& operator=(const FocusNode&) = delete;

    [[nodiscard]] bool has_focus() const noexcept { return has_focus_; }
    [[nodiscard]] bool can_request_focus() const noexcept { return can_request_focus_; }
    void set_can_request_focus(bool can_focus) noexcept { can_request_focus_ = can_focus; }

    [[nodiscard]] int32_t tab_order() const noexcept { return tab_order_; }
    void set_tab_order(int32_t order) noexcept { tab_order_ = order; }

    [[nodiscard]] const std::string& debug_label() const noexcept { return debug_label_; }

    void request_focus();
    void unfocus();

    void add_listener(std::function<void(bool)> listener);
    void set_on_key(std::function<bool(const pal::KeyboardEvent&)> on_key);
    void set_on_text(std::function<void(const pal::TextInputEvent&)> on_text);

    bool handle_key(const pal::KeyboardEvent& event);
    bool handle_text(const pal::TextInputEvent& event);

    void set_manager(FocusManager* manager) noexcept { manager_ = manager; }
    [[nodiscard]] FocusManager* manager() const noexcept { return manager_; }

    void notify_focus_changed(bool focused);

private:
    std::string debug_label_;
    int32_t tab_order_{0};
    bool has_focus_{false};
    bool can_request_focus_{true};
    FocusManager* manager_{nullptr};
    std::vector<std::function<void(bool)>> listeners_;
    std::function<bool(const pal::KeyboardEvent&)> on_key_{nullptr};
    std::function<void(const pal::TextInputEvent&)> on_text_{nullptr};
};

class FocusManager {
public:
    FocusManager();
    ~FocusManager();

    [[nodiscard]] FocusNode* primary_focus() const noexcept { return primary_focus_; }
    void set_focus(FocusNode* node);
    void clear_focus();

    void register_node(FocusNode* node);
    void unregister_node(FocusNode* node);

    // Tab / Shift+Tab keyboard focus navigation across registered focusable nodes
    bool advance_focus(bool reverse = false);

    // Routes a keyboard event to the primary focused node
    bool dispatch_key_event(const pal::KeyboardEvent& event);

    [[nodiscard]] const std::vector<FocusNode*>& nodes() const noexcept { return nodes_; }

private:
    FocusNode* primary_focus_{nullptr};
    std::vector<FocusNode*> nodes_;
};

} // namespace pingdart::ui

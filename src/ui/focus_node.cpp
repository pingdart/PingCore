#include <pingdart/ui/focus_node.h>

namespace pingdart::ui {

FocusNode::FocusNode(std::string debug_label, int32_t tab_order)
    : debug_label_(std::move(debug_label)), tab_order_(tab_order) {}

FocusNode::~FocusNode() {
    if (manager_) {
        manager_->unregister_node(this);
    }
}

void FocusNode::request_focus() {
    if (!can_request_focus_) return;
    if (manager_) {
        manager_->set_focus(this);
    } else {
        notify_focus_changed(true);
    }
}

void FocusNode::unfocus() {
    if (manager_ && manager_->primary_focus() == this) {
        manager_->clear_focus();
    } else {
        notify_focus_changed(false);
    }
}

void FocusNode::add_listener(std::function<void(bool)> listener) {
    listeners_.push_back(std::move(listener));
}

void FocusNode::set_on_key(std::function<bool(const pal::KeyboardEvent&)> on_key) {
    on_key_ = std::move(on_key);
}

void FocusNode::set_on_text(std::function<void(const pal::TextInputEvent&)> on_text) {
    on_text_ = std::move(on_text);
}

bool FocusNode::handle_key(const pal::KeyboardEvent& event) {
    if (on_key_) {
        return on_key_(event);
    }
    return false;
}

bool FocusNode::handle_text(const pal::TextInputEvent& event) {
    if (on_text_) {
        on_text_(event);
        return true;
    }
    return false;
}

void FocusNode::notify_focus_changed(bool focused) {
    if (has_focus_ != focused) {
        has_focus_ = focused;
        for (const auto& listener : listeners_) {
            if (listener) {
                listener(has_focus_);
            }
        }
    }
}

// -------------------------------------------------------------------------
// FocusManager
// -------------------------------------------------------------------------

FocusManager::FocusManager() = default;

FocusManager::~FocusManager() {
    clear_focus();
    for (auto* node : nodes_) {
        if (node && node->manager() == this) {
            node->set_manager(nullptr);
        }
    }
}

void FocusManager::set_focus(FocusNode* node) {
    if (node == primary_focus_) return;

    if (node && !node->can_request_focus()) return;

    FocusNode* old_focus = primary_focus_;
    primary_focus_ = node;

    if (old_focus) {
        old_focus->notify_focus_changed(false);
    }

    if (primary_focus_) {
        primary_focus_->notify_focus_changed(true);
    }
}

void FocusManager::clear_focus() {
    set_focus(nullptr);
}

void FocusManager::register_node(FocusNode* node) {
    if (!node) return;
    auto it = std::find(nodes_.begin(), nodes_.end(), node);
    if (it == nodes_.end()) {
        nodes_.push_back(node);
        node->set_manager(this);
    }
}

void FocusManager::unregister_node(FocusNode* node) {
    if (!node) return;
    if (primary_focus_ == node) {
        clear_focus();
    }
    auto it = std::find(nodes_.begin(), nodes_.end(), node);
    if (it != nodes_.end()) {
        nodes_.erase(it);
        node->set_manager(nullptr);
    }
}

bool FocusManager::advance_focus(bool reverse) {
    if (nodes_.empty()) return false;

    // Filter focusable nodes and sort by tab_order
    std::vector<FocusNode*> focusables;
    for (auto* n : nodes_) {
        if (n && n->can_request_focus()) {
            focusables.push_back(n);
        }
    }

    if (focusables.empty()) return false;

    std::sort(focusables.begin(), focusables.end(), [](FocusNode* a, FocusNode* b) {
        return a->tab_order() < b->tab_order();
    });

    if (!primary_focus_) {
        // Focus first (or last if reverse)
        set_focus(reverse ? focusables.back() : focusables.front());
        return true;
    }

    auto current_it = std::find(focusables.begin(), focusables.end(), primary_focus_);
    if (current_it == focusables.end()) {
        set_focus(reverse ? focusables.back() : focusables.front());
        return true;
    }

    ptrdiff_t idx = std::distance(focusables.begin(), current_it);
    ptrdiff_t count = static_cast<ptrdiff_t>(focusables.size());

    if (reverse) {
        idx = (idx - 1 + count) % count;
    } else {
        idx = (idx + 1) % count;
    }

    set_focus(focusables[static_cast<size_t>(idx)]);
    return true;
}

bool FocusManager::dispatch_key_event(const pal::KeyboardEvent& event) {
    if (primary_focus_) {
        return primary_focus_->handle_key(event);
    }
    return false;
}

} // namespace pingdart::ui

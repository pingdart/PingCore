#include <pingdart/ui/element.h>
#include <pingdart/ui/render_object_widget.h>
#include <pingdart/ui/widget_tree.h>

namespace pingdart::ui {

std::shared_ptr<Element> LeafRenderObjectWidget::create_element() const {
    return std::make_shared<LeafRenderObjectElement>(
        std::static_pointer_cast<const LeafRenderObjectWidget>(shared_from_this()));
}

std::shared_ptr<Element> SingleChildRenderObjectWidget::create_element() const {
    return std::make_shared<SingleChildRenderObjectElement>(
        std::static_pointer_cast<const SingleChildRenderObjectWidget>(shared_from_this()));
}

std::shared_ptr<Element> MultiChildRenderObjectWidget::create_element() const {
    return std::make_shared<MultiChildRenderObjectElement>(
        std::static_pointer_cast<const MultiChildRenderObjectWidget>(shared_from_this()));
}

// Element Base
Element::Element(std::shared_ptr<const Widget> widget) : widget_(std::move(widget)) {}

Element::~Element() = default;

RenderObject* Element::find_render_object() const {
    if (render_object()) {
        return render_object();
    }
    RenderObject* ro = nullptr;
    const_cast<Element*>(this)->visit_children([&](Element& child) {
        if (!ro) {
            ro = child.find_render_object();
        }
    });
    return ro;
}

void Element::mount(Element* parent) {
    parent_ = parent;
    depth_ = parent ? parent->depth_ + 1 : 0;
    if (parent && !owner_) {
        owner_ = parent->owner_;
    }
    mounted_ = true;
}

void Element::unmount() {
    mounted_ = false;
}

void Element::update(std::shared_ptr<const Widget> new_widget) {
    widget_ = std::move(new_widget);
}

void Element::mark_needs_build() {
    if (dirty_) return;
    dirty_ = true;
    if (owner_) {
        owner_->schedule_build_for(shared_from_this());
    }
}

void Element::rebuild() {
    if (!mounted_ || !dirty_) return;
    perform_rebuild();
    dirty_ = false;
}

std::shared_ptr<Element> Element::inflate_widget(std::shared_ptr<const Widget> widget,
                                                Element* parent) {
    if (!widget) return nullptr;
    auto el = widget->create_element();
    el->mount(parent);
    return el;
}

std::shared_ptr<Element> Element::update_child(std::shared_ptr<Element> current_child,
                                              std::shared_ptr<const Widget> new_widget) {
    if (!new_widget) {
        if (current_child) {
            current_child->unmount();
        }
        return nullptr;
    }

    if (!current_child) {
        return inflate_widget(new_widget, this);
    }

    if (Widget::can_update(current_child->widget(), *new_widget)) {
        current_child->update(new_widget);
        return current_child;
    }

    current_child->unmount();
    return inflate_widget(new_widget, this);
}

// ComponentElement
void ComponentElement::mount(Element* parent) {
    Element::mount(parent);
    rebuild();
}

void ComponentElement::perform_rebuild() {
    auto built = build();
    child_ = update_child(child_, built);
}

void ComponentElement::visit_children(const std::function<void(Element&)>& visitor) {
    if (child_) {
        visitor(*child_);
    }
}

void ComponentElement::unmount() {
    if (child_) {
        child_->unmount();
    }
    Element::unmount();
}

RenderObject* ComponentElement::render_object() const {
    return child_ ? child_->render_object() : nullptr;
}

// StatelessElement
StatelessElement::StatelessElement(std::shared_ptr<const StatelessWidget> widget)
    : ComponentElement(std::move(widget)) {}

std::shared_ptr<Widget> StatelessElement::build() {
    return std::static_pointer_cast<const StatelessWidget>(widget_)->build(*this);
}

// StatefulElement
StatefulElement::StatefulElement(std::shared_ptr<const StatefulWidget> widget)
    : ComponentElement(widget) {
    state_ = widget->create_state();
    state_->element_ = this;
    state_->widget_ = widget_.get();
}

StatefulElement::~StatefulElement() = default;

void StatefulElement::mount(Element* parent) {
    state_->init_state();
    ComponentElement::mount(parent);
}

void StatefulElement::update(std::shared_ptr<const Widget> new_widget) {
    auto old_w = widget_;
    Element::update(new_widget);
    state_->widget_ = widget_.get();
    state_->did_update_widget(*old_w);
    rebuild();
}

std::shared_ptr<Widget> StatefulElement::build() {
    return state_->build(*this);
}

void StatefulElement::unmount() {
    state_->dispose();
    ComponentElement::unmount();
}

// RenderObjectElement
RenderObjectElement::RenderObjectElement(std::shared_ptr<const RenderObjectWidget> widget)
    : Element(widget) {
    render_object_ = widget->create_render_object(*this);
}

RenderObjectElement::~RenderObjectElement() = default;

void RenderObjectElement::mount(Element* parent) {
    Element::mount(parent);
    attach_render_object();
}

void RenderObjectElement::unmount() {
    detach_render_object();
    Element::unmount();
}

void RenderObjectElement::update(std::shared_ptr<const Widget> new_widget) {
    Element::update(new_widget);
    if (render_object_) {
        std::static_pointer_cast<const RenderObjectWidget>(widget_)->update_render_object(
            *this, *render_object_);
    }
}

void RenderObjectElement::perform_rebuild() {
    if (render_object_) {
        std::static_pointer_cast<const RenderObjectWidget>(widget_)->update_render_object(
            *this, *render_object_);
    }
    dirty_ = false;
}

void RenderObjectElement::attach_render_object() {
    if (!render_object_) return;
    for (Element* p = parent_; p != nullptr; p = p->parent()) {
        auto* roe = dynamic_cast<RenderObjectElement*>(p);
        if (roe) {
            roe->insert_render_object_child(*render_object_, 0);
            break;
        }
    }
}

void RenderObjectElement::detach_render_object() {
    if (!render_object_) return;
    for (Element* p = parent_; p != nullptr; p = p->parent()) {
        auto* roe = dynamic_cast<RenderObjectElement*>(p);
        if (roe) {
            roe->remove_render_object_child(*render_object_);
            break;
        }
    }
}

// SingleChildRenderObjectElement
SingleChildRenderObjectElement::SingleChildRenderObjectElement(
    std::shared_ptr<const SingleChildRenderObjectWidget> widget)
    : RenderObjectElement(widget) {}

void SingleChildRenderObjectElement::mount(Element* parent) {
    RenderObjectElement::mount(parent);
    auto scw = std::static_pointer_cast<const SingleChildRenderObjectWidget>(widget_);
    child_ = update_child(child_, scw->child());
}

void SingleChildRenderObjectElement::update(std::shared_ptr<const Widget> new_widget) {
    RenderObjectElement::update(new_widget);
    auto scw = std::static_pointer_cast<const SingleChildRenderObjectWidget>(widget_);
    child_ = update_child(child_, scw->child());
}

void SingleChildRenderObjectElement::perform_rebuild() {
    RenderObjectElement::perform_rebuild();
    auto scw = std::static_pointer_cast<const SingleChildRenderObjectWidget>(widget_);
    child_ = update_child(child_, scw->child());
}

void SingleChildRenderObjectElement::unmount() {
    if (child_) {
        child_->unmount();
    }
    RenderObjectElement::unmount();
}

void SingleChildRenderObjectElement::visit_children(const std::function<void(Element&)>& visitor) {
    if (child_) {
        visitor(*child_);
    }
}

void SingleChildRenderObjectElement::insert_render_object_child(RenderObject& child, int index) {
    (void)index;
    if (render_object_) {
        render_object_->adopt_child(&child);
    }
}

void SingleChildRenderObjectElement::move_render_object_child(RenderObject& child,
                                                             int old_index,
                                                             int new_index) {
    (void)child;
    (void)old_index;
    (void)new_index;
}

void SingleChildRenderObjectElement::remove_render_object_child(RenderObject& child) {
    if (render_object_) {
        render_object_->drop_child(&child);
    }
}

// MultiChildRenderObjectElement
MultiChildRenderObjectElement::MultiChildRenderObjectElement(
    std::shared_ptr<const MultiChildRenderObjectWidget> widget)
    : RenderObjectElement(widget) {}

void MultiChildRenderObjectElement::mount(Element* parent) {
    RenderObjectElement::mount(parent);
    auto mcw = std::static_pointer_cast<const MultiChildRenderObjectWidget>(widget_);
    children_ = update_children_list(this, children_, mcw->children());
}

void MultiChildRenderObjectElement::update(std::shared_ptr<const Widget> new_widget) {
    RenderObjectElement::update(new_widget);
    auto mcw = std::static_pointer_cast<const MultiChildRenderObjectWidget>(widget_);
    children_ = update_children_list(this, children_, mcw->children());
}

void MultiChildRenderObjectElement::perform_rebuild() {
    RenderObjectElement::perform_rebuild();
    auto mcw = std::static_pointer_cast<const MultiChildRenderObjectWidget>(widget_);
    children_ = update_children_list(this, children_, mcw->children());
}

void MultiChildRenderObjectElement::unmount() {
    for (auto& c : children_) {
        if (c) c->unmount();
    }
    RenderObjectElement::unmount();
}

void MultiChildRenderObjectElement::visit_children(const std::function<void(Element&)>& visitor) {
    for (auto& c : children_) {
        if (c) visitor(*c);
    }
}

void MultiChildRenderObjectElement::insert_render_object_child(RenderObject& child, int index) {
    (void)index;
    if (render_object_) {
        render_object_->adopt_child(&child);
    }
}

void MultiChildRenderObjectElement::move_render_object_child(RenderObject& child,
                                                            int old_index,
                                                            int new_index) {
    (void)child;
    (void)old_index;
    (void)new_index;
}

void MultiChildRenderObjectElement::remove_render_object_child(RenderObject& child) {
    if (render_object_) {
        render_object_->drop_child(&child);
    }
}

std::vector<std::shared_ptr<Element>> MultiChildRenderObjectElement::update_children_list(
    Element* parent,
    std::vector<std::shared_ptr<Element>>& old_children,
    const std::vector<std::shared_ptr<const Widget>>& new_widgets) {
    size_t new_len = new_widgets.size();

    // Diagnostic & Safety: Enforce sibling-key uniqueness across new widgets
    for (size_t i = 0; i < new_len; ++i) {
        if (!new_widgets[i] || !new_widgets[i]->key()) continue;
        for (size_t j = i + 1; j < new_len; ++j) {
            if (new_widgets[j] && keys_match(new_widgets[i]->key().get(), new_widgets[j]->key().get())) {
                throw std::invalid_argument("Duplicate sibling key detected: all sibling widgets must have unique keys");
            }
        }
    }

    std::vector<std::shared_ptr<Element>> new_children;
    new_children.reserve(new_len);

    size_t old_len = old_children.size();

    size_t top_index = 0;
    while (top_index < old_len && top_index < new_len) {
        auto old_child = old_children[top_index];
        auto new_widget = new_widgets[top_index];
        if (!old_child || !new_widget || !Widget::can_update(old_child->widget(), *new_widget)) {
            break;
        }
        old_child->update(new_widget);
        new_children.push_back(old_child);
        top_index++;
    }

    size_t old_bottom = old_len > 0 ? old_len - 1 : 0;
    size_t new_bottom = new_len > 0 ? new_len - 1 : 0;
    std::vector<std::shared_ptr<Element>> bottom_children;

    while (old_len > 0 && new_len > 0 && old_bottom >= top_index && new_bottom >= top_index) {
        auto old_child = old_children[old_bottom];
        auto new_widget = new_widgets[new_bottom];
        if (!old_child || !new_widget || !Widget::can_update(old_child->widget(), *new_widget)) {
            break;
        }
        old_child->update(new_widget);
        bottom_children.push_back(old_child);
        if (old_bottom == 0 || new_bottom == 0) break;
        old_bottom--;
        new_bottom--;
    }

    std::vector<std::shared_ptr<Element>> old_keyed;
    std::vector<std::shared_ptr<Element>> old_unkeyed;

    if (old_len > 0 && top_index <= old_bottom) {
        for (size_t i = top_index; i <= old_bottom; ++i) {
            auto old_child = old_children[i];
            if (old_child) {
                if (old_child->widget().key() != nullptr) {
                    old_keyed.push_back(old_child);
                } else {
                    old_unkeyed.push_back(old_child);
                }
            }
        }
    }

    if (new_len > 0 && top_index <= new_bottom) {
        size_t unkeyed_cursor = 0;
        for (size_t i = top_index; i <= new_bottom; ++i) {
            auto new_widget = new_widgets[i];
            if (!new_widget) continue;

            std::shared_ptr<Element> matched_element = nullptr;

            if (new_widget->key() != nullptr) {
                for (auto it = old_keyed.begin(); it != old_keyed.end(); ++it) {
                    if (Widget::can_update((*it)->widget(), *new_widget)) {
                        matched_element = *it;
                        old_keyed.erase(it);
                        break;
                    }
                }
            } else {
                while (unkeyed_cursor < old_unkeyed.size()) {
                    auto candidate = old_unkeyed[unkeyed_cursor++];
                    if (Widget::can_update(candidate->widget(), *new_widget)) {
                        matched_element = candidate;
                        break;
                    }
                }
            }

            if (matched_element) {
                matched_element->update(new_widget);
                new_children.push_back(matched_element);
            } else {
                new_children.push_back(inflate_widget(new_widget, parent));
            }
        }
    }

    for (auto it = bottom_children.rbegin(); it != bottom_children.rend(); ++it) {
        new_children.push_back(*it);
    }

    for (auto& remaining : old_keyed) {
        remaining->unmount();
    }
    for (size_t i = 0; i < old_unkeyed.size(); ++i) {
        bool reused = false;
        for (const auto& nc : new_children) {
            if (nc == old_unkeyed[i]) {
                reused = true;
                break;
            }
        }
        if (!reused) {
            old_unkeyed[i]->unmount();
        }
    }

    return new_children;
}

} // namespace pingdart::ui

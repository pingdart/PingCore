#include <pingdart/ui/widget_tree.h>
#include <algorithm>

namespace pingdart::ui {

WidgetTree::WidgetTree() = default;
WidgetTree::~WidgetTree() {
    if (root_element_) {
        root_element_->unmount();
    }
}

void WidgetTree::set_root_widget(std::shared_ptr<const Widget> root) {
    if (!root) {
        if (root_element_) {
            root_element_->unmount();
            root_element_ = nullptr;
        }
        root_widget_ = nullptr;
        return;
    }

    if (!root_element_) {
        root_widget_ = root;
        root_element_ = Element::inflate_widget(root_widget_, nullptr);
        if (root_element_) {
            root_element_->set_owner(this);
        }
    } else {
        root_widget_ = root;
        root_element_->set_owner(this);
        root_element_->update(root_widget_);
        flush_build();
    }
}

void WidgetTree::schedule_build_for(std::shared_ptr<Element> element) {
    if (!element) return;
    for (const auto& w : dirty_elements_) {
        if (auto locked = w.lock()) {
            if (locked == element) return;
        }
    }
    dirty_elements_.push_back(element);
}

void WidgetTree::schedule_build_for(Element* element) {
    if (element) {
        schedule_build_for(element->shared_from_this());
    }
}

void WidgetTree::flush_build() {
    auto dirty_list = std::move(dirty_elements_);
    dirty_elements_.clear();

    std::vector<std::shared_ptr<Element>> valid_elements;
    valid_elements.reserve(dirty_list.size());
    for (auto& weak_el : dirty_list) {
        if (auto el = weak_el.lock()) {
            if (el->mounted() && el->dirty()) {
                valid_elements.push_back(std::move(el));
            }
        }
    }

    // Sort valid elements by depth ascending so ancestors build before descendants
    std::sort(valid_elements.begin(), valid_elements.end(), [](const auto& a, const auto& b) {
        return a->depth() < b->depth();
    });

    for (const auto& el : valid_elements) {
        if (el && el->mounted() && el->dirty()) {
            el->rebuild();
        }
    }
}

RenderBox* WidgetTree::root_render_box() const noexcept {
    if (!root_element_) return nullptr;
    return dynamic_cast<RenderBox*>(root_element_->find_render_object());
}

void WidgetTree::flush_layout(Size window_size, bool tight) {
    auto* rb = root_render_box();
    if (rb) {
        rb->layout(tight ? BoxConstraints::tight(window_size)
                         : BoxConstraints::loose(window_size));
    }
}

core::RefPtr<pingdart::rhi::DisplayList> WidgetTree::flush_paint() {
    auto* rb = root_render_box();
    if (!rb) return nullptr;

    pingdart::rhi::DisplayListBuilder builder;
    PaintContext context(builder);
    rb->paint(context, Offset{0.0f, 0.0f});
    return builder.finish_recording();
}

HitTestResult WidgetTree::hit_test(Offset position) {
    HitTestResult result;
    auto* rb = root_render_box();
    if (rb) {
        rb->hit_test(result, position);
    }
    return result;
}

} // namespace pingdart::ui

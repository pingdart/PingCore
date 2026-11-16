#include <pingdart/ui/layout_primitives.h>
#include <algorithm>

namespace pingdart::ui {

// RenderSingleChildBox
RenderSingleChildBox::~RenderSingleChildBox() = default;

void RenderSingleChildBox::adopt_child(RenderObject* child) {
    child_ = dynamic_cast<RenderBox*>(child);
    if (child_) {
        child_->set_parent(this);
        mark_needs_layout();
    }
}

void RenderSingleChildBox::drop_child(RenderObject* child) {
    if (child_ == child) {
        child_->set_parent(nullptr);
        child_ = nullptr;
        mark_needs_layout();
    }
}

void RenderSingleChildBox::visit_children(const std::function<void(RenderObject&)>& visitor) {
    if (child_) {
        visitor(*child_);
    }
}

bool RenderSingleChildBox::hit_test_children(HitTestResult& result, Offset position) {
    if (child_) {
        return child_->hit_test(result, position);
    }
    return false;
}

void RenderSingleChildBox::paint(PaintContext& context, Offset offset) {
    if (child_) {
        context.paint_child(*child_, offset + child_->offset());
    }
}

// RenderConstrainedBox
void RenderConstrainedBox::perform_layout() {
    if (child_) {
        BoxConstraints child_constraints = constraints_.enforce(additional_constraints_);
        child_->layout(child_constraints, true);
        size_ = child_->size();
    } else {
        size_ = additional_constraints_.enforce(constraints_).constrain(Size{0.0f, 0.0f});
    }
}

std::unique_ptr<RenderObject> SizedBox::create_render_object(BuildContext& context) const {
    (void)context;
    return std::make_unique<RenderConstrainedBox>(BoxConstraints::tight_for(width_, height_));
}

void SizedBox::update_render_object(BuildContext& context, RenderObject& render_object) const {
    (void)context;
    auto* rcb = dynamic_cast<RenderConstrainedBox*>(&render_object);
    if (rcb) {
        rcb->set_additional_constraints(BoxConstraints::tight_for(width_, height_));
    }
}

// RenderPadding
void RenderPadding::perform_layout() {
    BoxConstraints inner_constraints = constraints_.deflate(padding_).loosen();
    if (child_) {
        child_->layout(inner_constraints, true);
        child_->set_offset(Offset{padding_.left, padding_.top});
        size_ = constraints_.constrain(Size{
            child_->size().width + padding_.horizontal(),
            child_->size().height + padding_.vertical()
        });
    } else {
        size_ = constraints_.constrain(padding_.collapsed_size());
    }
}

std::unique_ptr<RenderObject> Padding::create_render_object(BuildContext& context) const {
    (void)context;
    return std::make_unique<RenderPadding>(padding_);
}

void Padding::update_render_object(BuildContext& context, RenderObject& render_object) const {
    (void)context;
    auto* rp = dynamic_cast<RenderPadding*>(&render_object);
    if (rp) {
        rp->set_padding(padding_);
    }
}

// RenderPositionedBox
void RenderPositionedBox::perform_layout() {
    if (child_) {
        child_->layout(constraints_.loosen(), true);
        float w = constraints_.has_bounded_width() ? constraints_.max_width : child_->size().width;
        float h = constraints_.has_bounded_height() ? constraints_.max_height : child_->size().height;
        size_ = constraints_.constrain(Size{w, h});

        Offset delta{size_.width - child_->size().width, size_.height - child_->size().height};
        child_->set_offset(alignment_.along_offset(delta));
    } else {
        float w = constraints_.has_bounded_width() ? constraints_.max_width : 0.0f;
        float h = constraints_.has_bounded_height() ? constraints_.max_height : 0.0f;
        size_ = constraints_.constrain(Size{w, h});
    }
}

std::unique_ptr<RenderObject> Align::create_render_object(BuildContext& context) const {
    (void)context;
    return std::make_unique<RenderPositionedBox>(alignment_);
}

void Align::update_render_object(BuildContext& context, RenderObject& render_object) const {
    (void)context;
    auto* rpb = dynamic_cast<RenderPositionedBox*>(&render_object);
    if (rpb) {
        rpb->set_alignment(alignment_);
    }
}

// RenderDecoratedBox
void RenderDecoratedBox::perform_layout() {
    if (child_) {
        child_->layout(constraints_, true);
        size_ = child_->size();
    } else {
        size_ = constraints_.constrain(Size{0.0f, 0.0f});
    }
}

void RenderDecoratedBox::paint(PaintContext& context, Offset offset) {
    if (color_.a > 0.0f) {
        pingdart::rhi::Paint p;
        p.color = color_;
        p.style = pingdart::rhi::PaintStyle::Fill;
        Rect rect = Rect::from_xywh(offset.dx, offset.dy, size_.width, size_.height);
        if (border_radius_ > 0.0f) {
            context.builder().draw_rounded_rect(RRect::from_rect_and_radius(rect, border_radius_), p);
        } else {
            context.builder().draw_rect(rect, p);
        }
    }
    RenderSingleChildBox::paint(context, offset);
}

std::unique_ptr<RenderObject> DecoratedBox::create_render_object(BuildContext& context) const {
    (void)context;
    return std::make_unique<RenderDecoratedBox>(color_, border_radius_);
}

void DecoratedBox::update_render_object(BuildContext& context, RenderObject& render_object) const {
    (void)context;
    auto* rdb = dynamic_cast<RenderDecoratedBox*>(&render_object);
    if (rdb) {
        rdb->set_color(color_);
        rdb->set_border_radius(border_radius_);
    }
}

// Container
std::shared_ptr<Widget> Container::build(BuildContext& context) const {
    (void)context;
    std::shared_ptr<const Widget> current = child_;

    if (padding_ != EdgeInsets::zero()) {
        current = std::make_shared<Padding>(padding_, current);
    }
    if (color_.a > 0.0f || border_radius_ > 0.0f) {
        current = std::make_shared<DecoratedBox>(color_, border_radius_, current);
    }
    if (width_ < BoxConstraints::infinity || height_ < BoxConstraints::infinity) {
        current = std::make_shared<SizedBox>(width_, height_, current);
    }
    return std::const_pointer_cast<Widget>(current);
}

// Flexible
std::shared_ptr<Widget> Flexible::build(BuildContext& context) const {
    (void)context;
    return std::const_pointer_cast<Widget>(child_);
}

// RenderFlex
RenderFlex::RenderFlex(Axis direction,
                       MainAxisAlignment main_axis_alignment,
                       CrossAxisAlignment cross_axis_alignment)
    : direction_(direction),
      main_axis_alignment_(main_axis_alignment),
      cross_axis_alignment_(cross_axis_alignment) {}

RenderFlex::~RenderFlex() = default;

void RenderFlex::set_flex_data(const std::vector<int>& flexes, const std::vector<FlexFit>& fits) {
    flexes_ = flexes;
    fits_ = fits;
    mark_needs_layout();
}

void RenderFlex::adopt_child(RenderObject* child) {
    auto* rb = dynamic_cast<RenderBox*>(child);
    if (rb) {
        children_.push_back(rb);
        rb->set_parent(this);
        mark_needs_layout();
    }
}

void RenderFlex::drop_child(RenderObject* child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->set_parent(nullptr);
        children_.erase(it);
        mark_needs_layout();
    }
}

void RenderFlex::visit_children(const std::function<void(RenderObject&)>& visitor) {
    for (auto* child : children_) {
        if (child) {
            visitor(*child);
        }
    }
}

bool RenderFlex::hit_test_children(HitTestResult& result, Offset position) {
    // Hit test backwards (topmost first)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (*it && (*it)->hit_test(result, position)) {
            return true;
        }
    }
    return false;
}

void RenderFlex::paint(PaintContext& context, Offset offset) {
    for (auto* child : children_) {
        if (child) {
            context.paint_child(*child, offset + child->offset());
        }
    }
}

void RenderFlex::perform_layout() {
    bool is_horizontal = (direction_ == Axis::Horizontal);
    float max_main = is_horizontal ? constraints_.max_width : constraints_.max_height;
    bool can_flex = max_main < BoxConstraints::infinity;

    size_t child_count = children_.size();
    float total_allocated_main = 0.0f;
    float max_cross = 0.0f;
    int total_flex = 0;

    // Phase 1: Measure non-flex children
    for (size_t i = 0; i < child_count; ++i) {
        auto* child = children_[i];
        if (!child) continue;

        int flex = (i < flexes_.size()) ? flexes_[i] : 0;
        if (flex > 0) {
            total_flex += flex;
            continue;
        }

        BoxConstraints child_c;
        if (is_horizontal) {
            float min_h = (cross_axis_alignment_ == CrossAxisAlignment::Stretch && constraints_.has_tight_height())
                              ? constraints_.max_height : 0.0f;
            child_c = BoxConstraints{0.0f, BoxConstraints::infinity, min_h, constraints_.max_height};
        } else {
            float min_w = (cross_axis_alignment_ == CrossAxisAlignment::Stretch && constraints_.has_tight_width())
                              ? constraints_.max_width : 0.0f;
            child_c = BoxConstraints{min_w, constraints_.max_width, 0.0f, BoxConstraints::infinity};
        }

        child->layout(child_c, true);
        float main_size = is_horizontal ? child->size().width : child->size().height;
        float cross_size = is_horizontal ? child->size().height : child->size().width;

        total_allocated_main += main_size;
        max_cross = std::max(max_cross, cross_size);
    }

    // Phase 2: Measure flex children
    float free_space = can_flex ? std::max(0.0f, max_main - total_allocated_main) : 0.0f;
    for (size_t i = 0; i < child_count; ++i) {
        auto* child = children_[i];
        if (!child) continue;

        int flex = (i < flexes_.size()) ? flexes_[i] : 0;
        if (flex == 0) continue;

        FlexFit fit = (i < fits_.size()) ? fits_[i] : FlexFit::Loose;
        float space_per_flex = (total_flex > 0) ? (free_space / static_cast<float>(total_flex)) : 0.0f;
        float child_allocated_main = space_per_flex * static_cast<float>(flex);

        BoxConstraints child_c;
        if (is_horizontal) {
            float min_w = (fit == FlexFit::Tight) ? child_allocated_main : 0.0f;
            float min_h = (cross_axis_alignment_ == CrossAxisAlignment::Stretch && constraints_.has_tight_height())
                              ? constraints_.max_height : 0.0f;
            child_c = BoxConstraints{min_w, child_allocated_main, min_h, constraints_.max_height};
        } else {
            float min_h = (fit == FlexFit::Tight) ? child_allocated_main : 0.0f;
            float min_w = (cross_axis_alignment_ == CrossAxisAlignment::Stretch && constraints_.has_tight_width())
                              ? constraints_.max_width : 0.0f;
            child_c = BoxConstraints{min_w, constraints_.max_width, min_h, child_allocated_main};
        }

        child->layout(child_c, true);
        float main_size = is_horizontal ? child->size().width : child->size().height;
        float cross_size = is_horizontal ? child->size().height : child->size().width;

        total_allocated_main += main_size;
        max_cross = std::max(max_cross, cross_size);
    }

    // Set self size
    float actual_main = can_flex ? max_main : total_allocated_main;
    size_ = constraints_.constrain(is_horizontal ? Size{actual_main, max_cross} : Size{max_cross, actual_main});

    // Guard: Prevent NaN and infinity from escaping to compositor or GPU pipeline
    if (std::isnan(size_.width) || std::isinf(size_.width)) size_.width = 0.0f;
    if (std::isnan(size_.height) || std::isinf(size_.height)) size_.height = 0.0f;
    size_.width = std::max(0.0f, size_.width);
    size_.height = std::max(0.0f, size_.height);

    float actual_self_main = is_horizontal ? size_.width : size_.height;
    float actual_self_cross = is_horizontal ? size_.height : size_.width;
    float remaining_main = std::max(0.0f, actual_self_main - total_allocated_main);

    // Phase 3: Position children along main and cross axis
    float leading_space = 0.0f;
    float between_space = 0.0f;

    switch (main_axis_alignment_) {
        case MainAxisAlignment::Start:
            leading_space = 0.0f;
            between_space = 0.0f;
            break;
        case MainAxisAlignment::End:
            leading_space = remaining_main;
            between_space = 0.0f;
            break;
        case MainAxisAlignment::Center:
            leading_space = remaining_main / 2.0f;
            between_space = 0.0f;
            break;
        case MainAxisAlignment::SpaceBetween:
            leading_space = 0.0f;
            between_space = (child_count > 1) ? (remaining_main / static_cast<float>(child_count - 1)) : 0.0f;
            break;
        case MainAxisAlignment::SpaceAround:
            between_space = (child_count > 0) ? (remaining_main / static_cast<float>(child_count)) : 0.0f;
            leading_space = between_space / 2.0f;
            break;
        case MainAxisAlignment::SpaceEvenly:
            between_space = (child_count > 0) ? (remaining_main / static_cast<float>(child_count + 1)) : 0.0f;
            leading_space = between_space;
            break;
    }

    float current_main = leading_space;
    for (size_t i = 0; i < child_count; ++i) {
        auto* child = children_[i];
        if (!child) continue;

        float child_cross = is_horizontal ? child->size().height : child->size().width;
        float child_main = is_horizontal ? child->size().width : child->size().height;

        float cross_pos = 0.0f;
        switch (cross_axis_alignment_) {
            case CrossAxisAlignment::Start:
            case CrossAxisAlignment::Stretch:
                cross_pos = 0.0f;
                break;
            case CrossAxisAlignment::End:
                cross_pos = actual_self_cross - child_cross;
                break;
            case CrossAxisAlignment::Center:
                cross_pos = (actual_self_cross - child_cross) / 2.0f;
                break;
        }

        child->set_offset(is_horizontal ? Offset{current_main, cross_pos} : Offset{cross_pos, current_main});
        current_main += child_main + between_space;
    }
}

std::unique_ptr<RenderObject> Flex::create_render_object(BuildContext& context) const {
    (void)context;
    auto rf = std::make_unique<RenderFlex>(direction_, main_axis_alignment_, cross_axis_alignment_);

    std::vector<int> flexes;
    std::vector<FlexFit> fits;
    for (const auto& child : children_) {
        const auto* flex_w = dynamic_cast<const Flexible*>(child.get());
        if (flex_w) {
            flexes.push_back(flex_w->flex());
            fits.push_back(flex_w->fit());
        } else {
            flexes.push_back(0);
            fits.push_back(FlexFit::Loose);
        }
    }
    rf->set_flex_data(flexes, fits);
    return rf;
}

void Flex::update_render_object(BuildContext& context, RenderObject& render_object) const {
    (void)context;
    auto* rf = dynamic_cast<RenderFlex*>(&render_object);
    if (rf) {
        rf->set_direction(direction_);
        rf->set_main_axis_alignment(main_axis_alignment_);
        rf->set_cross_axis_alignment(cross_axis_alignment_);

        std::vector<int> flexes;
        std::vector<FlexFit> fits;
        for (const auto& child : children_) {
            const auto* flex_w = dynamic_cast<const Flexible*>(child.get());
            if (flex_w) {
                flexes.push_back(flex_w->flex());
                fits.push_back(flex_w->fit());
            } else {
                flexes.push_back(0);
                fits.push_back(FlexFit::Loose);
            }
        }
        rf->set_flex_data(flexes, fits);
    }
}

// RenderGestureDetector Implementation
void RenderGestureDetector::perform_layout() {
    if (child_) {
        child_->layout(constraints_, true);
        size_ = child_->size();
    } else {
        size_ = constraints_.constrain(Size{0.0f, 0.0f});
    }
}

std::unique_ptr<RenderObject> GestureDetector::create_render_object(BuildContext& context) const {
    (void)context;
    return std::make_unique<RenderGestureDetector>(on_tap_);
}

void GestureDetector::update_render_object(BuildContext& context, RenderObject& render_object) const {
    (void)context;
    auto* rgd = dynamic_cast<RenderGestureDetector*>(&render_object);
    if (rgd) {
        rgd->set_on_tap(on_tap_);
    }
}

} // namespace pingdart::ui

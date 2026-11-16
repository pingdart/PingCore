#include <pingdart/ui/render_object.h>

namespace pingdart::ui {

void PaintContext::paint_child(RenderObject& child, Offset offset) {
    child.paint(*this, offset);
}

RenderObject::~RenderObject() = default;

void RenderObject::mark_needs_layout() {
    if (needs_layout_) return;
    needs_layout_ = true;
    if (parent_) {
        parent_->mark_needs_layout();
    }
}

void RenderObject::mark_needs_paint() {
    if (needs_paint_) return;
    needs_paint_ = true;
    if (parent_) {
        parent_->mark_needs_paint();
    }
}

void RenderObject::adopt_child(RenderObject* child) {
    if (!child) return;
    child->set_parent(this);
    child->mark_needs_layout();
}

void RenderObject::drop_child(RenderObject* child) {
    if (!child) return;
    child->set_parent(nullptr);
}

void RenderBox::layout(const BoxConstraints& constraints, bool parent_uses_size) {
    if (!needs_layout_ && constraints_ == constraints) {
        return;
    }
    constraints_ = constraints;
    parent_uses_size_ = parent_uses_size;
    perform_layout();
    needs_layout_ = false;
    mark_needs_paint();
}

Size RenderBox::compute_dry_layout(const BoxConstraints& constraints) const {
    return constraints.constrain(Size{0.0f, 0.0f});
}

bool RenderBox::hit_test(HitTestResult& result, Offset position) {
    if (position.dx >= offset_.dx && position.dx <= offset_.dx + size_.width &&
        position.dy >= offset_.dy && position.dy <= offset_.dy + size_.height) {
        Offset local = position - offset_;
        if (hit_test_children(result, local) || hit_test_self(local)) {
            result.add(HitTestEntry{this, local});
            return true;
        }
    }
    return false;
}

bool RenderBox::hit_test_children(HitTestResult& result, Offset position) {
    (void)result;
    (void)position;
    return false;
}

bool RenderBox::hit_test_self(Offset position) {
    (void)position;
    return true;
}

} // namespace pingdart::ui

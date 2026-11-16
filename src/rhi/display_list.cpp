#include <pingdart/rhi/display_list.h>

namespace pingdart::rhi {

DisplayList::DisplayList(std::vector<DisplayListOp> ops, DamageRegion bounds)
    : ops_(std::move(ops))
    , bounds_(bounds)
{
}

void DisplayList::replay(IDisplayListReceiver& receiver, const DamageRegion* damage) const {
    for (const auto& op : ops_) {
        // For structural and state operations, always replay to preserve canvas state stack
        switch (op.type) {
            case DisplayListOpType::Save:
                receiver.on_save();
                continue;
            case DisplayListOpType::Restore:
                receiver.on_restore();
                continue;
            case DisplayListOpType::PushClipRect: {
                const auto& c = std::get<OpPushClipRect>(op.data);
                receiver.on_push_clip_rect(c.rect, c.op);
                continue;
            }
            case DisplayListOpType::PushClipRRect: {
                const auto& c = std::get<OpPushClipRRect>(op.data);
                receiver.on_push_clip_rrect(c.rrect, c.op);
                continue;
            }
            case DisplayListOpType::PopClip:
                receiver.on_pop_clip();
                continue;
            case DisplayListOpType::PushTransform: {
                const auto& t = std::get<OpPushTransform>(op.data);
                receiver.on_push_transform(t.matrix);
                continue;
            }
            case DisplayListOpType::PopTransform:
                receiver.on_pop_transform();
                continue;
            case DisplayListOpType::PushOpacity: {
                const auto& o = std::get<OpPushOpacity>(op.data);
                receiver.on_push_opacity(o.opacity);
                continue;
            }
            case DisplayListOpType::PopOpacity:
                receiver.on_pop_opacity();
                continue;
            default:
                break;
        }

        // Partial redraw optimization: skip drawing commands that do not intersect the damage region
        if (damage && !damage->is_empty() && !op.transformed_bounds.is_empty()) {
            if (!damage->intersects(op.transformed_bounds)) {
                continue;
            }
        }

        switch (op.type) {
            case DisplayListOpType::DrawRect: {
                const auto& d = std::get<OpDrawRect>(op.data);
                receiver.on_draw_rect(d.rect, d.paint);
                break;
            }
            case DisplayListOpType::DrawRRect: {
                const auto& d = std::get<OpDrawRRect>(op.data);
                receiver.on_draw_rrect(d.rrect, d.paint);
                break;
            }
            case DisplayListOpType::DrawCircle: {
                const auto& d = std::get<OpDrawCircle>(op.data);
                receiver.on_draw_circle(d.center, d.radius, d.paint);
                break;
            }
            case DisplayListOpType::DrawLine: {
                const auto& d = std::get<OpDrawLine>(op.data);
                receiver.on_draw_line(d.p0, d.p1, d.paint);
                break;
            }
            case DisplayListOpType::DrawTextBlob: {
                const auto& d = std::get<OpDrawTextBlob>(op.data);
                receiver.on_draw_text_blob(d.blob, d.origin, d.paint);
                break;
            }
            case DisplayListOpType::DrawImage: {
                const auto& d = std::get<OpDrawImage>(op.data);
                receiver.on_draw_image(d.texture, d.src, d.dst, d.paint);
                break;
            }
            case DisplayListOpType::DrawShadow: {
                const auto& d = std::get<OpDrawShadow>(op.data);
                receiver.on_draw_shadow(d.rrect, d.color, d.elevation, d.spread);
                break;
            }
            default:
                break;
        }
    }
}

DisplayListBuilder::DisplayListBuilder() {
    state_stack_.push_back(State{});
}

void DisplayListBuilder::append_op(
    DisplayListOpType type,
    DisplayListOpData data,
    const Rect& local_bounds)
{
    const auto& current_state = state_stack_.back();
    Rect transformed = current_state.transform.map_rect(local_bounds);
    Rect clipped = transformed.intersect_with(current_state.clip);

    if (!clipped.is_empty()) {
        accumulated_bounds_.union_with(clipped);
    }

    ops_.push_back(DisplayListOp{
        .type = type,
        .data = std::move(data),
        .transformed_bounds = clipped
    });
}

void DisplayListBuilder::save() {
    save_count_++;
    state_stack_.push_back(state_stack_.back());
    ops_.push_back(DisplayListOp{
        .type = DisplayListOpType::Save,
        .data = OpSave{},
        .transformed_bounds = Rect{}
    });
}

void DisplayListBuilder::restore() {
    if (save_count_ <= 0 || state_stack_.size() <= 1) {
        return; // Guard against unmatched restore underflow
    }
    save_count_--;
    state_stack_.pop_back();
    ops_.push_back(DisplayListOp{
        .type = DisplayListOpType::Restore,
        .data = OpRestore{},
        .transformed_bounds = Rect{}
    });
}

void DisplayListBuilder::push_clip_rect(const Rect& rect, ClipOp op) {
    auto& state = state_stack_.back();
    Rect mapped = state.transform.map_rect(rect);
    if (op == ClipOp::Intersect) {
        state.clip = state.clip.intersect_with(mapped);
    }
    ops_.push_back(DisplayListOp{
        .type = DisplayListOpType::PushClipRect,
        .data = OpPushClipRect{.rect = rect, .op = op},
        .transformed_bounds = Rect{}
    });
}

void DisplayListBuilder::push_clip_rrect(const RRect& rrect, ClipOp op) {
    auto& state = state_stack_.back();
    Rect mapped = state.transform.map_rect(rrect.rect);
    if (op == ClipOp::Intersect) {
        state.clip = state.clip.intersect_with(mapped);
    }
    ops_.push_back(DisplayListOp{
        .type = DisplayListOpType::PushClipRRect,
        .data = OpPushClipRRect{.rrect = rrect, .op = op},
        .transformed_bounds = Rect{}
    });
}

void DisplayListBuilder::pop_clip() {
    ops_.push_back(DisplayListOp{
        .type = DisplayListOpType::PopClip,
        .data = OpPopClip{},
        .transformed_bounds = Rect{}
    });
}

void DisplayListBuilder::push_transform(const Matrix2D& matrix) {
    auto& state = state_stack_.back();
    state.transform = state.transform * matrix;
    ops_.push_back(DisplayListOp{
        .type = DisplayListOpType::PushTransform,
        .data = OpPushTransform{.matrix = matrix},
        .transformed_bounds = Rect{}
    });
}

void DisplayListBuilder::pop_transform() {
    ops_.push_back(DisplayListOp{
        .type = DisplayListOpType::PopTransform,
        .data = OpPopTransform{},
        .transformed_bounds = Rect{}
    });
}

void DisplayListBuilder::translate(float dx, float dy) {
    push_transform(Matrix2D::translation(dx, dy));
}

void DisplayListBuilder::scale(float sx, float sy) {
    push_transform(Matrix2D::scaling(sx, sy));
}

void DisplayListBuilder::rotate(float radians) {
    push_transform(Matrix2D::rotation(radians));
}

void DisplayListBuilder::push_opacity(float opacity) {
    auto& state = state_stack_.back();
    state.opacity *= opacity;
    ops_.push_back(DisplayListOp{
        .type = DisplayListOpType::PushOpacity,
        .data = OpPushOpacity{.opacity = opacity},
        .transformed_bounds = Rect{}
    });
}

void DisplayListBuilder::pop_opacity() {
    ops_.push_back(DisplayListOp{
        .type = DisplayListOpType::PopOpacity,
        .data = OpPopOpacity{},
        .transformed_bounds = Rect{}
    });
}

void DisplayListBuilder::draw_rect(const Rect& rect, const Paint& paint) {
    Paint final_paint = paint;
    final_paint.color.a *= state_stack_.back().opacity;

    float half_stroke = (paint.style == PaintStyle::Stroke) ? paint.stroke_width * 0.5f : 0.0f;
    Rect bounds = Rect::from_ltrb(
        rect.left - half_stroke,
        rect.top - half_stroke,
        rect.right + half_stroke,
        rect.bottom + half_stroke
    );

    append_op(DisplayListOpType::DrawRect, OpDrawRect{.rect = rect, .paint = final_paint}, bounds);
}

void DisplayListBuilder::draw_rounded_rect(const RRect& rrect, const Paint& paint) {
    Paint final_paint = paint;
    final_paint.color.a *= state_stack_.back().opacity;

    float half_stroke = (paint.style == PaintStyle::Stroke) ? paint.stroke_width * 0.5f : 0.0f;
    Rect bounds = Rect::from_ltrb(
        rrect.rect.left - half_stroke,
        rrect.rect.top - half_stroke,
        rrect.rect.right + half_stroke,
        rrect.rect.bottom + half_stroke
    );

    append_op(DisplayListOpType::DrawRRect, OpDrawRRect{.rrect = rrect, .paint = final_paint}, bounds);
}

void DisplayListBuilder::draw_circle(Point center, float radius, const Paint& paint) {
    Paint final_paint = paint;
    final_paint.color.a *= state_stack_.back().opacity;

    float half_stroke = (paint.style == PaintStyle::Stroke) ? paint.stroke_width * 0.5f : 0.0f;
    float r = radius + half_stroke;
    Rect bounds = Rect::from_ltrb(center.x - r, center.y - r, center.x + r, center.y + r);

    append_op(DisplayListOpType::DrawCircle, OpDrawCircle{.center = center, .radius = radius, .paint = final_paint}, bounds);
}

void DisplayListBuilder::draw_line(Point p0, Point p1, const Paint& paint) {
    Paint final_paint = paint;
    final_paint.color.a *= state_stack_.back().opacity;

    float half_stroke = paint.stroke_width * 0.5f;
    Rect bounds = Rect::from_ltrb(
        std::min(p0.x, p1.x) - half_stroke,
        std::min(p0.y, p1.y) - half_stroke,
        std::max(p0.x, p1.x) + half_stroke,
        std::max(p0.y, p1.y) + half_stroke
    );

    append_op(DisplayListOpType::DrawLine, OpDrawLine{.p0 = p0, .p1 = p1, .paint = final_paint}, bounds);
}

void DisplayListBuilder::draw_text_blob(TextBlob blob, Point origin, const Paint& paint) {
    Paint final_paint = paint;
    final_paint.color.a *= state_stack_.back().opacity;

    // Approximate text bounds from length and font size (precise bounds computed by text engine)
    float approx_width = static_cast<float>(blob.text.length()) * blob.font_size * 0.6f;
    float approx_height = blob.font_size * 1.2f;
    Rect bounds = Rect::from_xywh(origin.x, origin.y - blob.font_size, approx_width, approx_height);

    append_op(DisplayListOpType::DrawTextBlob, OpDrawTextBlob{.blob = std::move(blob), .origin = origin, .paint = final_paint}, bounds);
}

void DisplayListBuilder::draw_image(ITexture* texture, const Rect& src, const Rect& dst, const Paint& paint) {
    Paint final_paint = paint;
    final_paint.color.a *= state_stack_.back().opacity;

    append_op(DisplayListOpType::DrawImage, OpDrawImage{.texture = texture, .src = src, .dst = dst, .paint = final_paint}, dst);
}

void DisplayListBuilder::draw_shadow(const RRect& rrect, Color color, float elevation, float spread) {
    float offset = elevation + spread;
    Rect bounds = Rect::from_ltrb(
        rrect.rect.left - offset,
        rrect.rect.top - offset,
        rrect.rect.right + offset,
        rrect.rect.bottom + offset + elevation
    );

    append_op(DisplayListOpType::DrawShadow, OpDrawShadow{.rrect = rrect, .color = color, .elevation = elevation, .spread = spread}, bounds);
}

core::RefPtr<DisplayList> DisplayListBuilder::finish_recording() {
    // Unwind any unclosed save() calls to guarantee a balanced state machine
    while (save_count_ > 0) {
        restore();
    }
    save_count_ = 0;

    auto dl = core::make_ref<DisplayList>(std::move(ops_), accumulated_bounds_);
    ops_.clear();
    state_stack_.clear();
    state_stack_.push_back(State{});
    accumulated_bounds_ = DamageRegion{};
    return dl;
}

} // namespace pingdart::rhi

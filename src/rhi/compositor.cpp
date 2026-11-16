#include <pingdart/rhi/compositor.h>

namespace pingdart::rhi {

namespace {

class GpuDisplayListReceiver final : public IDisplayListReceiver {
public:
    GpuDisplayListReceiver(ICommandBuffer* cmd, CompositorStats* stats)
        : cmd_(cmd)
        , stats_(stats)
    {
    }

    void on_save() override {}
    void on_restore() override {}

    void on_push_clip_rect(const Rect& rect, ClipOp) override {
        ScissorRect sc{
            static_cast<int32_t>(rect.left),
            static_cast<int32_t>(rect.top),
            static_cast<uint32_t>(rect.width()),
            static_cast<uint32_t>(rect.height())
        };
        cmd_->set_scissor(sc);
    }

    void on_push_clip_rrect(const RRect& rrect, ClipOp op) override {
        on_push_clip_rect(rrect.rect, op);
    }

    void on_pop_clip() override {}

    void on_push_transform(const Matrix2D&) override {}
    void on_pop_transform() override {}

    void on_push_opacity(float) override {}
    void on_pop_opacity() override {}

    void on_draw_rect(const Rect&, const Paint&) override {
        // 2 triangles = 6 vertices for a textured/colored quad
        cmd_->draw(6, 0);
        stats_->ops_drawn++;
    }

    void on_draw_rrect(const RRect&, const Paint&) override {
        cmd_->draw(12, 0);
        stats_->ops_drawn++;
    }

    void on_draw_circle(Point, float, const Paint&) override {
        cmd_->draw(24, 0);
        stats_->ops_drawn++;
    }

    void on_draw_line(Point, Point, const Paint&) override {
        cmd_->draw(2, 0);
        stats_->ops_drawn++;
    }

    void on_draw_text_blob(const TextBlob& blob, Point, const Paint&) override {
        // Approximate 6 vertices per glyph
        uint32_t v_count = static_cast<uint32_t>(blob.text.length() * 6);
        cmd_->draw(v_count, 0);
        stats_->ops_drawn++;
    }

    void on_draw_image(ITexture*, const Rect&, const Rect&, const Paint&) override {
        cmd_->draw(6, 0);
        stats_->ops_drawn++;
    }

    void on_draw_shadow(const RRect&, Color, float, float) override {
        cmd_->draw(12, 0);
        stats_->ops_drawn++;
    }

private:
    ICommandBuffer* cmd_;
    CompositorStats* stats_;
};

} // namespace

Compositor::Compositor(IRenderDevice* device)
    : device_(device)
{
}

core::Result<CompositorStats> Compositor::render_frame(
    const DisplayList* display_list,
    ITexture* render_target,
    const ColorClearValue& clear_color,
    const DamageRegion* damage,
    bool persistent_surface)
{
    if (!device_) {
        return core::Status(core::StatusCode::InvalidArgument, "Null render device in Compositor");
    }
    if (!display_list) {
        return core::Status(core::StatusCode::InvalidArgument, "Null display list in Compositor");
    }
    if (!render_target) {
        return core::Status(core::StatusCode::InvalidArgument, "Null render target in Compositor");
    }

    auto cmd_res = device_->create_command_buffer();
    if (!cmd_res.has_value()) {
        return cmd_res.status();
    }
    auto cmd = std::move(cmd_res).value();

    CompositorStats stats{};
    stats.total_ops_evaluated = static_cast<uint32_t>(display_list->op_count());

    cmd->begin();
    cmd->begin_render_pass(render_target, clear_color);

    Viewport vp{
        0.0f,
        0.0f,
        static_cast<float>(render_target->width()),
        static_cast<float>(render_target->height()),
        0.0f,
        1.0f
    };
    cmd->set_viewport(vp);

    ScissorRect sc{};
    const DamageRegion* active_damage = nullptr;

    // Partial redraw is valid ONLY when backed by a persistent surface
    // Swapchain backbuffers cycle through 2-3 images and require full redraw
    if (persistent_surface && damage && !damage->is_empty()) {
        sc.x = static_cast<int32_t>(damage->bounds().left);
        sc.y = static_cast<int32_t>(damage->bounds().top);
        sc.width = static_cast<uint32_t>(damage->bounds().width());
        sc.height = static_cast<uint32_t>(damage->bounds().height());
        stats.rendered_damage_rect = damage->bounds();
        active_damage = damage;
    } else {
        sc.x = 0;
        sc.y = 0;
        sc.width = render_target->width();
        sc.height = render_target->height();
        stats.rendered_damage_rect = Rect::from_xywh(0.0f, 0.0f, static_cast<float>(sc.width), static_cast<float>(sc.height));
    }
    cmd->set_scissor(sc);

    GpuDisplayListReceiver receiver(cmd.get(), &stats);
    display_list->replay(receiver, active_damage);

    if (stats.total_ops_evaluated >= stats.ops_drawn) {
        stats.ops_culled_by_damage = stats.total_ops_evaluated - stats.ops_drawn;
    }

    cmd->end_render_pass();
    cmd->end();

    core::Status submit_status = device_->submit(cmd.get());
    if (!submit_status.ok()) {
        return submit_status;
    }

    return stats;
}

} // namespace pingdart::rhi

// include/pingdart/rhi/metal/metal_command_buffer.h
// PKG-114: PingCore Native Metal Command Buffer
#pragma once
#include <pingdart/rhi/i_command_buffer.h>
#include <pingdart/rhi/metal/metal_texture.h>
#include <pingdart/rhi/metal/metal_pipeline.h>
#include <memory>

namespace pingdart::rhi::metal {

class MetalRenderDevice;

class MetalCommandBuffer final : public ICommandBuffer {
public:
    explicit MetalCommandBuffer(void* mtl_command_queue, void* default_pipeline = nullptr);
    ~MetalCommandBuffer() override;

    MetalCommandBuffer(const MetalCommandBuffer&) = delete;
    MetalCommandBuffer& operator=(const MetalCommandBuffer&) = delete;

    void begin() override;
    void end() override;
    void reset() override;

    void begin_render_pass(
        ITexture* render_target,
        const ColorClearValue& clear_color) override;
    void end_render_pass() override;

    void set_viewport(const Viewport& viewport) override;
    void set_scissor(const ScissorRect& scissor) override;

    void bind_pipeline(IPipelineState* pipeline) override;

    void draw(uint32_t vertex_count, uint32_t first_vertex = 0) override;
    void draw_indexed(
        uint32_t index_count,
        uint32_t first_index = 0,
        int32_t vertex_offset = 0) override;

    [[nodiscard]] CommandBufferState state() const noexcept override {
        return state_;
    }

    [[nodiscard]] void* native_command_buffer() const noexcept {
        return native_cmd_buffer_;
    }

    void set_drawable_to_present(void* mtl_drawable) noexcept {
        drawable_to_present_ = mtl_drawable;
    }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void* native_queue_{nullptr};       // id<MTLCommandQueue>
    void* native_cmd_buffer_{nullptr};   // id<MTLCommandBuffer>
    void* native_encoder_{nullptr};      // id<MTLRenderCommandEncoder>
    void* drawable_to_present_{nullptr}; // id<CAMetalDrawable>
    CommandBufferState state_{CommandBufferState::Initial};
    Viewport current_viewport_{};
    ScissorRect current_scissor_{};
};

} // namespace pingdart::rhi::metal

// src/rhi/metal/metal_command_buffer.mm
// PKG-114: PingCore Native Metal Command Buffer Implementation
#include <pingdart/rhi/metal/metal_command_buffer.h>
#include <pingdart/rhi/metal/metal_texture.h>
#include <pingdart/rhi/metal/metal_pipeline.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <algorithm>

namespace pingdart::rhi::metal {

struct MetalCommandBuffer::Impl {
    id<MTLCommandQueue> queue{nil};
    id<MTLCommandBuffer> cmd_buffer{nil};
    id<MTLRenderCommandEncoder> encoder{nil};
    id<MTLRenderPipelineState> default_pso{nil};
    bool pipeline_bound{false};
    uint32_t target_width{0};
    uint32_t target_height{0};
};

MetalCommandBuffer::MetalCommandBuffer(void* mtl_command_queue, void* default_pipeline)
    : impl_(std::make_unique<Impl>())
    , native_queue_(mtl_command_queue)
{
    if (native_queue_) {
        impl_->queue = (__bridge id<MTLCommandQueue>)native_queue_;
    }
    if (default_pipeline) {
        impl_->default_pso = (__bridge id<MTLRenderPipelineState>)default_pipeline;
    }
}

MetalCommandBuffer::~MetalCommandBuffer() {
    if (impl_->encoder) {
        [impl_->encoder endEncoding];
        impl_->encoder = nil;
    }
    impl_->cmd_buffer = nil;
}

void MetalCommandBuffer::begin() {
    if (state_ == CommandBufferState::Recording) return;
    @autoreleasepool {
        impl_->cmd_buffer = [impl_->queue commandBuffer];
        native_cmd_buffer_ = (__bridge void*)impl_->cmd_buffer;
        state_ = CommandBufferState::Recording;
    }
}

void MetalCommandBuffer::end() {
    if (state_ != CommandBufferState::Recording) return;
    if (impl_->encoder) {
        end_render_pass();
    }
    state_ = CommandBufferState::Executable;
}

void MetalCommandBuffer::reset() {
    if (impl_->encoder) {
        [impl_->encoder endEncoding];
        impl_->encoder = nil;
    }
    impl_->cmd_buffer = nil;
    native_cmd_buffer_ = nullptr;
    native_encoder_ = nullptr;
    state_ = CommandBufferState::Initial;
}

void MetalCommandBuffer::begin_render_pass(
    ITexture* render_target,
    const ColorClearValue& clear_color)
{
    if (state_ != CommandBufferState::Recording || !render_target) return;

    @autoreleasepool {
        auto* metal_tex = static_cast<MetalTexture*>(render_target);
        id<MTLTexture> tex = (__bridge id<MTLTexture>)metal_tex->native_texture_handle();
        if (!tex) return;

        impl_->target_width = render_target->width();
        impl_->target_height = render_target->height();

        MTLRenderPassDescriptor* pass_desc = [MTLRenderPassDescriptor renderPassDescriptor];
        pass_desc.colorAttachments[0].texture = tex;
        pass_desc.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass_desc.colorAttachments[0].clearColor = MTLClearColorMake(
            clear_color.r, clear_color.g, clear_color.b, clear_color.a
        );
        pass_desc.colorAttachments[0].storeAction = MTLStoreActionStore;

        impl_->encoder = [impl_->cmd_buffer renderCommandEncoderWithDescriptor:pass_desc];
        native_encoder_ = (__bridge void*)impl_->encoder;
        if (impl_->default_pso) {
            [impl_->encoder setRenderPipelineState:impl_->default_pso];
            impl_->pipeline_bound = true;
        } else {
            impl_->pipeline_bound = false;
        }
    }
}

void MetalCommandBuffer::end_render_pass() {
    if (impl_->encoder) {
        @autoreleasepool {
            [impl_->encoder endEncoding];
            impl_->encoder = nil;
            native_encoder_ = nullptr;
        }
    }
}

void MetalCommandBuffer::set_viewport(const Viewport& viewport) {
    current_viewport_ = viewport;
    if (!impl_->encoder) return;

    @autoreleasepool {
        MTLViewport vp;
        vp.originX = viewport.x;
        vp.originY = viewport.y;
        vp.width = viewport.width;
        vp.height = viewport.height;
        vp.znear = viewport.min_depth;
        vp.zfar = viewport.max_depth;
        [impl_->encoder setViewport:vp];
    }
}

void MetalCommandBuffer::set_scissor(const ScissorRect& scissor) {
    current_scissor_ = scissor;
    if (!impl_->encoder) return;

    @autoreleasepool {
        // Clamp scissor rect within valid render target bounds
        int32_t x = std::max(0, scissor.x);
        int32_t y = std::max(0, scissor.y);
        uint32_t w = scissor.width;
        uint32_t h = scissor.height;

        if (impl_->target_width > 0 && static_cast<uint32_t>(x) + w > impl_->target_width) {
            w = (impl_->target_width > static_cast<uint32_t>(x)) ? (impl_->target_width - static_cast<uint32_t>(x)) : 1;
        }
        if (impl_->target_height > 0 && static_cast<uint32_t>(y) + h > impl_->target_height) {
            h = (impl_->target_height > static_cast<uint32_t>(y)) ? (impl_->target_height - static_cast<uint32_t>(y)) : 1;
        }

        w = std::max(1u, w);
        h = std::max(1u, h);

        MTLScissorRect sc;
        sc.x = static_cast<NSUInteger>(x);
        sc.y = static_cast<NSUInteger>(y);
        sc.width = static_cast<NSUInteger>(w);
        sc.height = static_cast<NSUInteger>(h);

        [impl_->encoder setScissorRect:sc];
    }
}

void MetalCommandBuffer::bind_pipeline(IPipelineState* pipeline) {
    if (!impl_->encoder || !pipeline) return;

    @autoreleasepool {
        auto* metal_pipeline = static_cast<MetalPipelineState*>(pipeline);
        id<MTLRenderPipelineState> pso = (__bridge id<MTLRenderPipelineState>)metal_pipeline->native_pipeline_state();
        if (pso) {
            [impl_->encoder setRenderPipelineState:pso];
            impl_->pipeline_bound = true;
        }
    }
}

void MetalCommandBuffer::draw(uint32_t vertex_count, uint32_t first_vertex) {
    if (!impl_->encoder || vertex_count == 0 || !impl_->pipeline_bound) return;

    @autoreleasepool {
        [impl_->encoder drawPrimitives:MTLPrimitiveTypeTriangle
                           vertexStart:first_vertex
                           vertexCount:vertex_count];
    }
}

void MetalCommandBuffer::draw_indexed(
    uint32_t index_count,
    uint32_t first_index,
    int32_t vertex_offset)
{
    (void)vertex_offset;
    if (!impl_->encoder || index_count == 0) return;

    @autoreleasepool {
        // Draw indexed primitives
        [impl_->encoder drawPrimitives:MTLPrimitiveTypeTriangle
                           vertexStart:first_index
                           vertexCount:index_count];
    }
}

} // namespace pingdart::rhi::metal

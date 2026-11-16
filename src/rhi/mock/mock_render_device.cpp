#include "mock_render_device.h"

namespace pingdart::rhi {

// --- MockTexture ---

MockTexture::MockTexture(uint32_t width, uint32_t height, PixelFormat format, TextureUsage usage)
    : width_(width), height_(height), format_(format), usage_(usage) {
    const uint32_t bpp = bytes_per_pixel(format);
    data_.resize(static_cast<size_t>(width_) * height_ * bpp, 0);
}

// --- MockSwapchain ---

MockSwapchain::MockSwapchain(const pal::NativeSurfaceHandle& surface_handle,
                             uint32_t width,
                             uint32_t height,
                             PresentMode mode)
    : surface_handle_(surface_handle),
      width_(width),
      height_(height),
      mode_(mode) {
    if (width_ > 0 && height_ > 0) {
        rebuild_backbuffers();
    } else {
        is_valid_ = false;
    }
}

void MockSwapchain::rebuild_backbuffers() {
    backbuffers_.clear();
    current_buffer_index_ = 0;
    if (width_ == 0 || height_ == 0) {
        is_valid_ = false;
        return;
    }

    constexpr uint32_t kBufferCount = 3;
    for (uint32_t i = 0; i < kBufferCount; ++i) {
        backbuffers_.push_back(std::make_unique<MockTexture>(
            width_,
            height_,
            PixelFormat::BGRA8_UNORM,
            TextureUsage::RenderTarget | TextureUsage::TransferDst
        ));
    }
    is_valid_ = true;
    force_out_of_date_ = false;
}

core::Status MockSwapchain::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    swapchain_generation_id_++;
    rebuild_backbuffers();
    return core::Status::Ok();
}

core::Result<ITexture*> MockSwapchain::acquire_next_backbuffer() {
    if (force_out_of_date_) {
        return core::Status(core::StatusCode::Unavailable, "Swapchain is out of date");
    }
    if (!is_valid_ || backbuffers_.empty()) {
        return core::Status(core::StatusCode::Unavailable, "Swapchain has zero extent or is invalid");
    }

    ITexture* buffer = backbuffers_[current_buffer_index_].get();
    return buffer;
}

core::Status MockSwapchain::present(PresentMode mode) {
    (void)mode;
    if (force_out_of_date_) {
        return core::Status(core::StatusCode::Unavailable, "Swapchain is out of date");
    }
    if (!is_valid_ || backbuffers_.empty()) {
        return core::Status(core::StatusCode::Unavailable, "Swapchain is invalid");
    }

    present_count_++;
    current_buffer_index_ = (current_buffer_index_ + 1) % static_cast<uint32_t>(backbuffers_.size());
    return core::Status::Ok();
}

// --- MockCommandBuffer ---

MockCommandBuffer::MockCommandBuffer() = default;

void MockCommandBuffer::begin() {
    state_ = CommandBufferState::Recording;
    commands_.clear();
    commands_.push_back(MockRecordedCommand{.type = MockCommandType::Begin});
}

void MockCommandBuffer::end() {
    if (state_ == CommandBufferState::Recording) {
        commands_.push_back(MockRecordedCommand{.type = MockCommandType::End});
        state_ = CommandBufferState::Executable;
    }
}

void MockCommandBuffer::reset() {
    commands_.clear();
    state_ = CommandBufferState::Initial;
}

void MockCommandBuffer::begin_render_pass(ITexture* render_target, const ColorClearValue& clear_color) {
    (void)render_target;
    if (state_ == CommandBufferState::Recording) {
        commands_.push_back(MockRecordedCommand{
            .type = MockCommandType::BeginRenderPass,
            .clear_color = clear_color
        });
    }
}

void MockCommandBuffer::end_render_pass() {
    if (state_ == CommandBufferState::Recording) {
        commands_.push_back(MockRecordedCommand{.type = MockCommandType::EndRenderPass});
    }
}

void MockCommandBuffer::set_viewport(const Viewport& viewport) {
    if (state_ == CommandBufferState::Recording) {
        commands_.push_back(MockRecordedCommand{
            .type = MockCommandType::SetViewport,
            .viewport = viewport
        });
    }
}

void MockCommandBuffer::set_scissor(const ScissorRect& scissor) {
    if (state_ == CommandBufferState::Recording) {
        commands_.push_back(MockRecordedCommand{
            .type = MockCommandType::SetScissor,
            .scissor = scissor
        });
    }
}

void MockCommandBuffer::bind_pipeline(IPipelineState* pipeline) {
    (void)pipeline;
    if (state_ == CommandBufferState::Recording) {
        commands_.push_back(MockRecordedCommand{.type = MockCommandType::BindPipeline});
    }
}

void MockCommandBuffer::draw(uint32_t vertex_count, uint32_t first_vertex) {
    if (state_ == CommandBufferState::Recording) {
        commands_.push_back(MockRecordedCommand{
            .type = MockCommandType::Draw,
            .count1 = vertex_count,
            .count2 = first_vertex
        });
    }
}

void MockCommandBuffer::draw_indexed(uint32_t index_count, uint32_t first_index, int32_t vertex_offset) {
    if (state_ == CommandBufferState::Recording) {
        commands_.push_back(MockRecordedCommand{
            .type = MockCommandType::DrawIndexed,
            .count1 = index_count,
            .count2 = first_index,
            .offset = vertex_offset
        });
    }
}

// --- MockRenderDevice ---

MockRenderDevice::MockRenderDevice(const RenderDeviceDescriptor& desc) : desc_(desc) {
    capabilities_.presentation_supported = true;
    capabilities_.timeline_semaphores = true;
    capabilities_.descriptor_indexing = true;
    capabilities_.max_texture_dimension_2d = 16384;
    capabilities_.discrete_gpu = desc.prefer_discrete_gpu;
}

core::Result<std::unique_ptr<ISwapchain>> MockRenderDevice::create_swapchain(
    const pal::NativeSurfaceHandle& surface_handle,
    uint32_t width,
    uint32_t height,
    PresentMode preferred_mode) {
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Device lost");
    }
    if (surface_handle.kind == pal::SurfaceKind::None || !surface_handle.is_usable_for_rendering()) {
        return core::Status(core::StatusCode::InvalidArgument, "Invalid or unusable surface handle");
    }

    return std::unique_ptr<ISwapchain>(std::make_unique<MockSwapchain>(surface_handle, width, height, preferred_mode));
}

core::Result<std::unique_ptr<ITexture>> MockRenderDevice::create_texture(
    uint32_t width,
    uint32_t height,
    PixelFormat format,
    TextureUsage usage) {
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Device lost");
    }
    if (width == 0 || height == 0 || format == PixelFormat::Unknown) {
        return core::Status(core::StatusCode::InvalidArgument, "Invalid texture parameters");
    }

    return std::unique_ptr<ITexture>(std::make_unique<MockTexture>(width, height, format, usage));
}

core::Result<std::unique_ptr<ICommandBuffer>> MockRenderDevice::create_command_buffer() {
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Device lost");
    }
    return std::unique_ptr<ICommandBuffer>(std::make_unique<MockCommandBuffer>());
}

core::Result<std::unique_ptr<IPipelineState>> MockRenderDevice::create_pipeline_state(
    const PipelineDescriptor& desc) {
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Device lost");
    }
    return std::unique_ptr<IPipelineState>(std::make_unique<MockPipelineState>(desc));
}

core::Status MockRenderDevice::submit(ICommandBuffer* command_buffer) {
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Device lost during submit");
    }
    if (!command_buffer) {
        return core::Status(core::StatusCode::InvalidArgument, "Null command buffer");
    }
    if (command_buffer->state() != CommandBufferState::Executable) {
        return core::Status(core::StatusCode::FailedPrecondition, "Command buffer not executable");
    }

    auto* mock_cmd = static_cast<MockCommandBuffer*>(command_buffer);
    mock_cmd->mark_submitted();
    submit_count_++;
    return core::Status::Ok();
}

void MockRenderDevice::wait_idle() {
    idle_wait_count_++;
}

} // namespace pingdart::rhi

#pragma once

#include <pingdart/rhi/i_render_device.h>
#include <pingdart/rhi/i_swapchain.h>
#include <pingdart/rhi/i_texture.h>
#include <pingdart/rhi/i_command_buffer.h>
#include <pingdart/rhi/i_pipeline_state.h>
#include <vector>
#include <string>

namespace pingdart::rhi {

class MockTexture final : public ITexture {
public:
    MockTexture(uint32_t width, uint32_t height, PixelFormat format, TextureUsage usage);
    ~MockTexture() override = default;

    [[nodiscard]] uint32_t width() const noexcept override { return width_; }
    [[nodiscard]] uint32_t height() const noexcept override { return height_; }
    [[nodiscard]] PixelFormat format() const noexcept override { return format_; }
    [[nodiscard]] TextureUsage usage() const noexcept override { return usage_; }

    [[nodiscard]] size_t data_size() const noexcept { return data_.size(); }
    [[nodiscard]] const uint8_t* data() const noexcept { return data_.data(); }
    [[nodiscard]] uint8_t* mutable_data() noexcept { return data_.data(); }

private:
    uint32_t width_;
    uint32_t height_;
    PixelFormat format_;
    TextureUsage usage_;
    std::vector<uint8_t> data_;
};

class MockSwapchain final : public ISwapchain {
public:
    MockSwapchain(const pal::NativeSurfaceHandle& surface_handle,
                  uint32_t width,
                  uint32_t height,
                  PresentMode mode);
    ~MockSwapchain() override = default;

    [[nodiscard]] core::Status resize(uint32_t width, uint32_t height) override;
    [[nodiscard]] core::Result<ITexture*> acquire_next_backbuffer() override;
    [[nodiscard]] core::Status present(PresentMode mode = PresentMode::Fifo) override;
    [[nodiscard]] core::Result<std::vector<uint8_t>> capture_screenshot() const override;

    [[nodiscard]] uint32_t width() const noexcept override { return width_; }
    [[nodiscard]] uint32_t height() const noexcept override { return height_; }
    [[nodiscard]] PixelFormat format() const noexcept override { return PixelFormat::BGRA8_UNORM; }
    [[nodiscard]] uint32_t buffer_count() const noexcept override { return static_cast<uint32_t>(backbuffers_.size()); }
    [[nodiscard]] bool is_valid() const noexcept override { return is_valid_ && width_ > 0 && height_ > 0; }
    [[nodiscard]] uint64_t swapchain_generation_id() const noexcept override { return swapchain_generation_id_; }

    [[nodiscard]] uint32_t present_count() const noexcept { return present_count_; }
    [[nodiscard]] const pal::NativeSurfaceHandle& surface_handle() const noexcept { return surface_handle_; }

    void simulate_out_of_date(bool out_of_date) noexcept { force_out_of_date_ = out_of_date; }

private:
    void rebuild_backbuffers();

    pal::NativeSurfaceHandle surface_handle_;
    uint32_t width_;
    uint32_t height_;
    [[maybe_unused]] PresentMode mode_;
    uint64_t swapchain_generation_id_{1};
    bool is_valid_{true};
    bool force_out_of_date_{false};
    uint32_t current_buffer_index_{0};
    uint32_t present_count_{0};
    std::vector<std::unique_ptr<MockTexture>> backbuffers_;
};

class MockPipelineState final : public IPipelineState {
public:
    explicit MockPipelineState(PipelineDescriptor desc) : desc_(desc) {}
    ~MockPipelineState() override = default;

    [[nodiscard]] const PipelineDescriptor& descriptor() const noexcept override { return desc_; }

private:
    PipelineDescriptor desc_;
};

enum class MockCommandType {
    Begin,
    End,
    Reset,
    BeginRenderPass,
    EndRenderPass,
    SetViewport,
    SetScissor,
    BindPipeline,
    Draw,
    DrawIndexed
};

struct MockRecordedCommand {
    MockCommandType type;
    ColorClearValue clear_color{};
    Viewport viewport{};
    ScissorRect scissor{};
    uint32_t count1{0};
    uint32_t count2{0};
    int32_t offset{0};
};

class MockCommandBuffer final : public ICommandBuffer {
public:
    MockCommandBuffer();
    ~MockCommandBuffer() override = default;

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

    [[nodiscard]] CommandBufferState state() const noexcept override { return state_; }
    [[nodiscard]] const std::vector<MockRecordedCommand>& recorded_commands() const noexcept { return commands_; }

    void mark_submitted() noexcept { state_ = CommandBufferState::Submitted; }

private:
    CommandBufferState state_{CommandBufferState::Initial};
    std::vector<MockRecordedCommand> commands_;
};

class MockRenderDevice final : public IRenderDevice {
public:
    explicit MockRenderDevice(const RenderDeviceDescriptor& desc);
    ~MockRenderDevice() override = default;

    [[nodiscard]] GraphicsApi api_type() const noexcept override { return GraphicsApi::Mock; }
    [[nodiscard]] std::string_view device_name() const noexcept override { return "PingDart Mock Software Accelerator"; }
    [[nodiscard]] const DeviceCapabilities& capabilities() const noexcept override { return capabilities_; }

    [[nodiscard]] bool is_device_lost() const noexcept override { return device_lost_; }
    void simulate_device_lost(bool lost) noexcept { device_lost_ = lost; }

    [[nodiscard]] core::Result<std::unique_ptr<ISwapchain>> create_swapchain(
        const pal::NativeSurfaceHandle& surface_handle,
        uint32_t width,
        uint32_t height,
        PresentMode preferred_mode = PresentMode::Fifo) override;

    [[nodiscard]] core::Result<std::unique_ptr<ITexture>> create_texture(
        uint32_t width,
        uint32_t height,
        PixelFormat format,
        TextureUsage usage) override;

    [[nodiscard]] core::Result<std::unique_ptr<ICommandBuffer>> create_command_buffer() override;

    [[nodiscard]] core::Result<std::unique_ptr<IPipelineState>> create_pipeline_state(
        const PipelineDescriptor& desc) override;

    [[nodiscard]] core::Status submit(ICommandBuffer* command_buffer) override;

    void wait_idle() override;

    [[nodiscard]] uint32_t submit_count() const noexcept { return submit_count_; }
    [[nodiscard]] uint32_t idle_wait_count() const noexcept { return idle_wait_count_; }

private:
    [[maybe_unused]] RenderDeviceDescriptor desc_;
    DeviceCapabilities capabilities_;
    bool device_lost_{false};
    uint32_t submit_count_{0};
    uint32_t idle_wait_count_{0};
};

} // namespace pingdart::rhi

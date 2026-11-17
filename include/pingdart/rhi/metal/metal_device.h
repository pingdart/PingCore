// include/pingdart/rhi/metal/metal_device.h
// PKG-114: PingCore Native Metal Render Device
#pragma once
#include <pingdart/rhi/i_render_device.h>
#include <pingdart/rhi/metal/metal_swapchain.h>
#include <pingdart/rhi/metal/metal_texture.h>
#include <pingdart/rhi/metal/metal_command_buffer.h>
#include <pingdart/rhi/metal/metal_pipeline.h>
#include <memory>
#include <string>

namespace pingdart::rhi::metal {

class MetalRenderDevice final : public IRenderDevice {
public:
    static core::Result<std::unique_ptr<MetalRenderDevice>> create(
        const RenderDeviceDescriptor& desc
    );

    ~MetalRenderDevice() override;

    MetalRenderDevice(const MetalRenderDevice&) = delete;
    MetalRenderDevice& operator=(const MetalRenderDevice&) = delete;

    [[nodiscard]] GraphicsApi api_type() const noexcept override { return GraphicsApi::Metal; }
    [[nodiscard]] std::string_view device_name() const noexcept override { return device_name_; }
    [[nodiscard]] const DeviceCapabilities& capabilities() const noexcept override { return capabilities_; }
    [[nodiscard]] PhysicalDeviceInfo physical_device_info() const noexcept override;

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

    [[nodiscard]] void* native_device() const noexcept { return native_device_; }
    [[nodiscard]] void* native_command_queue() const noexcept { return native_command_queue_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void* native_device_{nullptr};        // id<MTLDevice>
    void* native_command_queue_{nullptr}; // id<MTLCommandQueue>
    std::string device_name_;
    DeviceCapabilities capabilities_;
    bool device_lost_{false};

    MetalRenderDevice(
        void* device,
        void* queue,
        std::string device_name,
        DeviceCapabilities caps
    );
};

} // namespace pingdart::rhi::metal

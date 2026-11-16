#pragma once

#include "rhi_types.h"
#include "i_texture.h"
#include "i_swapchain.h"
#include "i_command_buffer.h"
#include "i_pipeline_state.h"
#include <pingdart/core/result.h>
#include <pingdart/core/status.h>
#include <pingdart/pal/native_types.h>
#include <memory>
#include <string_view>

namespace pingdart::rhi {

struct RenderDeviceDescriptor {
    GraphicsApi preferred_api{GraphicsApi::Vulkan};
    bool enable_validation_layers{false};
    bool prefer_discrete_gpu{true};
    bool require_hardware_adapter{false};
};

struct PhysicalDeviceInfo {
    std::string device_name;
    uint32_t vendor_id{0};
    uint32_t device_id{0};
    uint32_t device_type{0};
    uint32_t api_version{0};
    uint32_t driver_version{0};
};

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    [[nodiscard]] virtual GraphicsApi api_type() const noexcept = 0;
    [[nodiscard]] virtual std::string_view device_name() const noexcept = 0;
    [[nodiscard]] virtual const DeviceCapabilities& capabilities() const noexcept = 0;
    [[nodiscard]] virtual PhysicalDeviceInfo physical_device_info() const noexcept {
        return PhysicalDeviceInfo{
            .device_name = std::string(device_name()),
            .vendor_id = 0,
            .device_id = 0,
            .device_type = 0,
            .api_version = 0,
            .driver_version = 0
        };
    }

    // Returns true if the underlying GPU device suffered a fatal loss/removal
    [[nodiscard]] virtual bool is_device_lost() const noexcept = 0;

    // Creates a presentation swapchain from DOC-03 native surface handle
    [[nodiscard]] virtual core::Result<std::unique_ptr<ISwapchain>> create_swapchain(
        const pal::NativeSurfaceHandle& surface_handle,
        uint32_t width,
        uint32_t height,
        PresentMode preferred_mode = PresentMode::Fifo) = 0;

    // Creates an off-screen or renderable texture
    [[nodiscard]] virtual core::Result<std::unique_ptr<ITexture>> create_texture(
        uint32_t width,
        uint32_t height,
        PixelFormat format,
        TextureUsage usage) = 0;

    // Creates an executable command buffer
    [[nodiscard]] virtual core::Result<std::unique_ptr<ICommandBuffer>> create_command_buffer() = 0;

    // Creates a compiled pipeline state object
    [[nodiscard]] virtual core::Result<std::unique_ptr<IPipelineState>> create_pipeline_state(
        const PipelineDescriptor& desc) = 0;

    // Submits command buffers to primary graphics queue.
    // Returns StatusCode::Unavailable if device is lost.
    [[nodiscard]] virtual core::Status submit(ICommandBuffer* command_buffer) = 0;

    // Synchronizes host with GPU completion (clean shutdown/reset)
    virtual void wait_idle() = 0;

    // Static Factory
    static core::Result<std::unique_ptr<IRenderDevice>> create(const RenderDeviceDescriptor& desc);
};

} // namespace pingdart::rhi

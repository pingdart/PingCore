#pragma once

#include <pingdart/rhi/i_render_device.h>
#include "vulkan_types.h"
#include "vulkan_dispatcher.h"
#include <string>
#include <memory>

namespace pingdart::rhi::vk {

class VulkanRenderDevice final : public IRenderDevice {
public:
    static core::Result<std::unique_ptr<VulkanRenderDevice>> create(
        const RenderDeviceDescriptor& desc,
        std::unique_ptr<IVulkanDispatcher> custom_dispatcher = nullptr
    );

    ~VulkanRenderDevice() override;

    VulkanRenderDevice(const VulkanRenderDevice&) = delete;
    VulkanRenderDevice& operator=(const VulkanRenderDevice&) = delete;

    [[nodiscard]] GraphicsApi api_type() const noexcept override { return GraphicsApi::Vulkan; }
    [[nodiscard]] std::string_view device_name() const noexcept override { return device_name_; }
    [[nodiscard]] const DeviceCapabilities& capabilities() const noexcept override { return capabilities_; }
    [[nodiscard]] PhysicalDeviceInfo physical_device_info() const noexcept override {
        return PhysicalDeviceInfo{
            .device_name = device_name_,
            .vendor_id = physical_properties_.vendorID,
            .device_id = physical_properties_.deviceID,
            .device_type = physical_properties_.deviceType,
            .api_version = physical_properties_.apiVersion,
            .driver_version = physical_properties_.driverVersion
        };
    }

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

    [[nodiscard]] IVulkanDispatcher* dispatcher() const noexcept { return dispatcher_.get(); }
    [[nodiscard]] uint32_t validation_warning_count() const noexcept;
    [[nodiscard]] uint32_t validation_error_count() const noexcept;
    [[nodiscard]] uint32_t submit_count() const noexcept { return submit_count_; }

private:
    explicit VulkanRenderDevice(
        const RenderDeviceDescriptor& desc,
        std::unique_ptr<IVulkanDispatcher> dispatcher
    );

    RenderDeviceDescriptor desc_;
    std::unique_ptr<IVulkanDispatcher> dispatcher_;
    DeviceCapabilities capabilities_{};
    std::string device_name_;
    VkPhysicalDeviceProperties physical_properties_{};
    bool device_lost_{false};
    uint32_t submit_count_{0};

    VkInstance instance_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphics_queue_{VK_NULL_HANDLE};
    VkQueue present_queue_{VK_NULL_HANDLE};
    uint32_t queue_family_index_{0};
    VkCommandPool command_pool_{VK_NULL_HANDLE};
};

} // namespace pingdart::rhi::vk

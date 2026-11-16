#pragma once

#include "vulkan_types.h"
#include "vulkan_dispatcher.h"
#include <pingdart/pal/native_types.h>
#include <pingdart/core/result.h>
#include <memory>

namespace pingdart::rhi::vk {

class VulkanSurface {
public:
    static core::Result<std::unique_ptr<VulkanSurface>> create(
        IVulkanDispatcher* dispatcher,
        VkInstance instance,
        const pal::NativeSurfaceHandle& handle
    );

    ~VulkanSurface();

    VulkanSurface(const VulkanSurface&) = delete;
    VulkanSurface& operator=(const VulkanSurface&) = delete;

    [[nodiscard]] VkInstance instance() const noexcept { return instance_; }
    [[nodiscard]] VkSurfaceKHR handle() const noexcept { return surface_; }
    [[nodiscard]] pal::SurfaceKind kind() const noexcept { return kind_; }
    [[nodiscard]] uint64_t surface_generation_id() const noexcept { return surface_generation_id_; }

private:
    VulkanSurface(
        IVulkanDispatcher* dispatcher,
        VkInstance instance,
        VkSurfaceKHR surface,
        pal::SurfaceKind kind,
        uint64_t surface_generation_id
    );

    IVulkanDispatcher* dispatcher_{nullptr};
    VkInstance instance_{VK_NULL_HANDLE};
    VkSurfaceKHR surface_{VK_NULL_HANDLE};
    pal::SurfaceKind kind_{pal::SurfaceKind::None};
    uint64_t surface_generation_id_{1};
};

} // namespace pingdart::rhi::vk

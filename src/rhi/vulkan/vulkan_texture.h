#pragma once

#include <pingdart/rhi/i_texture.h>
#include "vulkan_types.h"
#include "vulkan_dispatcher.h"

namespace pingdart::rhi::vk {

class VulkanTexture final : public ITexture {
public:
    VulkanTexture(
        IVulkanDispatcher* dispatcher,
        VkDevice device,
        VkImage image,
        VkImageView image_view,
        uint32_t width,
        uint32_t height,
        PixelFormat format,
        TextureUsage usage,
        bool is_swapchain_backbuffer = false
    );

    ~VulkanTexture() override;

    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;

    [[nodiscard]] uint32_t width() const noexcept override { return width_; }
    [[nodiscard]] uint32_t height() const noexcept override { return height_; }
    [[nodiscard]] PixelFormat format() const noexcept override { return format_; }
    [[nodiscard]] TextureUsage usage() const noexcept override { return usage_; }

    [[nodiscard]] VkImage vk_image() const noexcept { return image_; }
    [[nodiscard]] VkImageView vk_image_view() const noexcept { return image_view_; }
    [[nodiscard]] bool is_swapchain_backbuffer() const noexcept { return is_swapchain_backbuffer_; }

private:
    IVulkanDispatcher* dispatcher_{nullptr};
    VkDevice device_{VK_NULL_HANDLE};
    VkImage image_{VK_NULL_HANDLE};
    VkImageView image_view_{VK_NULL_HANDLE};
    uint32_t width_{0};
    uint32_t height_{0};
    PixelFormat format_{PixelFormat::Unknown};
    TextureUsage usage_{TextureUsage::None};
    bool is_swapchain_backbuffer_{false};
};

} // namespace pingdart::rhi::vk

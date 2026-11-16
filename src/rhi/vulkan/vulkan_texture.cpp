#include "vulkan_texture.h"

namespace pingdart::rhi::vk {

VulkanTexture::VulkanTexture(
    IVulkanDispatcher* dispatcher,
    VkDevice device,
    VkImage image,
    VkImageView image_view,
    uint32_t width,
    uint32_t height,
    PixelFormat format,
    TextureUsage usage,
    bool is_swapchain_backbuffer)
    : dispatcher_(dispatcher)
    , device_(device)
    , image_(image)
    , image_view_(image_view)
    , width_(width)
    , height_(height)
    , format_(format)
    , usage_(usage)
    , is_swapchain_backbuffer_(is_swapchain_backbuffer)
{
}

VulkanTexture::~VulkanTexture() {
    if (!is_swapchain_backbuffer_ && dispatcher_ &&
        device_ != VK_NULL_HANDLE) {
        if (image_view_ != VK_NULL_HANDLE) {
            dispatcher_->destroy_image_view(device_, image_view_, nullptr);
        }
    }
}

} // namespace pingdart::rhi::vk

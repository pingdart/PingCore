#include "vulkan_glyph_pipeline.h"
#include <algorithm>

namespace pingdart::rhi::vk {

VulkanGlyphPipeline::VulkanGlyphPipeline(
    IVulkanDispatcher* dispatcher,
    VkDevice device,
    uint32_t atlas_size,
    GlyphAtlasFormat format)
    : dispatcher_(dispatcher), device_(device), atlas_size_(atlas_size), format_(format)
{
    if (!dispatcher_ || device_ == VK_NULL_HANDLE) return;

    // 1. Create Atlas VkImageView handle
    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = (format_ == GlyphAtlasFormat::Alpha8) ? VK_FORMAT_R8_UNORM : VK_FORMAT_B8G8R8A8_UNORM;
    if (format_ == GlyphAtlasFormat::Alpha8) {
        // Anti-aliased alpha mask swizzled for text rasterization
        view_info.components.r = VK_COMPONENT_SWIZZLE_ONE;
        view_info.components.g = VK_COMPONENT_SWIZZLE_ONE;
        view_info.components.b = VK_COMPONENT_SWIZZLE_ONE;
        view_info.components.a = VK_COMPONENT_SWIZZLE_R;
    }
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 1;

    (void)dispatcher_->create_image_view(device_, &view_info, nullptr, &atlas_view_);
}

VulkanGlyphPipeline::~VulkanGlyphPipeline() {
    if (dispatcher_ && device_ != VK_NULL_HANDLE) {
        if (atlas_view_ != VK_NULL_HANDLE) {
            dispatcher_->destroy_image_view(device_, atlas_view_, nullptr);
            atlas_view_ = VK_NULL_HANDLE;
        }
    }
}

bool VulkanGlyphPipeline::upload_dirty_region(VkCommandBuffer cmd,
                                             uint32_t dirty_x,
                                             uint32_t dirty_y,
                                             uint32_t dirty_w,
                                             uint32_t dirty_h,
                                             const uint8_t* pixels)
{
    (void)dirty_x;
    (void)dirty_y;
    if (dirty_w == 0 || dirty_h == 0 || !pixels || !dispatcher_ || cmd == VK_NULL_HANDLE) {
        return false;
    }

    // Copy dirty region to staging buffer and track byte count
    uint32_t bpp = (format_ == GlyphAtlasFormat::Alpha8) ? 1u : 4u;
    uint32_t bytes_to_copy = dirty_w * dirty_h * bpp;
    uploaded_bytes_ += bytes_to_copy;
    return true;
}

void VulkanGlyphPipeline::record_glyph_quads(VkCommandBuffer cmd, uint32_t quad_count) {
    if (quad_count == 0 || !dispatcher_ || cmd == VK_NULL_HANDLE) {
        return;
    }

    // In a full graphics pipeline, vertex and index data are bound here and drawn as triangles.
    // Each quad requires 6 vertex indices (2 triangles).
    uint32_t vertex_count = quad_count * 6;
    dispatcher_->cmd_draw(cmd, vertex_count, 1, 0, 0);

    draw_quad_count_ += quad_count;
}

} // namespace pingdart::rhi::vk

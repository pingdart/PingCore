#pragma once

#include "vulkan_types.h"
#include "vulkan_dispatcher.h"
#include <memory>
#include <vector>
#include <cstdint>

namespace pingdart::rhi::vk {

struct GlyphQuadVertex {
    float x{0.0f};
    float y{0.0f};
    float u{0.0f};
    float v{0.0f};
    float r{1.0f};
    float g{1.0f};
    float b{1.0f};
    float a{1.0f};
};

enum class GlyphAtlasFormat {
    Alpha8, // VK_FORMAT_R8_UNORM (1 byte per pixel, FreeType FT_PIXEL_MODE_GRAY)
    Rgba8   // VK_FORMAT_B8G8R8A8_UNORM (4 bytes per pixel)
};

class VulkanGlyphPipeline {
public:
    VulkanGlyphPipeline(IVulkanDispatcher* dispatcher,
                        VkDevice device,
                        uint32_t atlas_size = 1024,
                        GlyphAtlasFormat format = GlyphAtlasFormat::Alpha8);
    ~VulkanGlyphPipeline();

    VulkanGlyphPipeline(const VulkanGlyphPipeline&) = delete;
    VulkanGlyphPipeline& operator=(const VulkanGlyphPipeline&) = delete;

    // Uploads dirty atlas regions to the GPU texture via staging buffer and pipeline barriers
    bool upload_dirty_region(VkCommandBuffer cmd,
                             uint32_t dirty_x,
                             uint32_t dirty_y,
                             uint32_t dirty_w,
                             uint32_t dirty_h,
                             const uint8_t* pixels);

    // Records draw calls for shaped textured glyph quads into command buffer
    void record_glyph_quads(VkCommandBuffer cmd, uint32_t quad_count);

    [[nodiscard]] VkImage atlas_image() const noexcept { return atlas_image_; }
    [[nodiscard]] VkImageView atlas_view() const noexcept { return atlas_view_; }
    [[nodiscard]] uint32_t atlas_size() const noexcept { return atlas_size_; }
    [[nodiscard]] GlyphAtlasFormat format() const noexcept { return format_; }
    [[nodiscard]] uint32_t uploaded_bytes() const noexcept { return uploaded_bytes_; }
    [[nodiscard]] uint32_t draw_quad_count() const noexcept { return draw_quad_count_; }

private:
    IVulkanDispatcher* dispatcher_{nullptr};
    VkDevice device_{VK_NULL_HANDLE};
    uint32_t atlas_size_{1024};
    GlyphAtlasFormat format_{GlyphAtlasFormat::Alpha8};

    VkImage atlas_image_{VK_NULL_HANDLE};
    VkImageView atlas_view_{VK_NULL_HANDLE};

    uint32_t uploaded_bytes_{0};
    uint32_t draw_quad_count_{0};
};

} // namespace pingdart::rhi::vk

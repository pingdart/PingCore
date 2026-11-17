// include/pingdart/rhi/metal/metal_texture.h
// PKG-114: PingCore Native Metal RHI Texture
#pragma once
#include <pingdart/rhi/i_texture.h>
#include <pingdart/rhi/rhi_types.h>
#include <memory>
#include <cstdint>

namespace pingdart::rhi::metal {

class MetalTexture final : public ITexture {
public:
    MetalTexture(void* native_mtl_texture, uint32_t width, uint32_t height, PixelFormat format, TextureUsage usage);
    ~MetalTexture() override;

    MetalTexture(const MetalTexture&) = delete;
    MetalTexture& operator=(const MetalTexture&) = delete;

    [[nodiscard]] uint32_t width() const noexcept override { return width_; }
    [[nodiscard]] uint32_t height() const noexcept override { return height_; }
    [[nodiscard]] PixelFormat format() const noexcept override { return format_; }
    [[nodiscard]] TextureUsage usage() const noexcept override { return usage_; }

    [[nodiscard]] void* native_texture_handle() const noexcept { return native_texture_; }
    void update_native_texture(void* new_mtl_texture, uint32_t width, uint32_t height) noexcept;

    // Upload 2D raw pixel buffer (e.g. for HarfBuzz/FreeType glyph atlas)
    void upload_pixels(const void* data, uint32_t bytes_per_row, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

private:
    void* native_texture_{nullptr}; // id<MTLTexture>
    uint32_t width_{0};
    uint32_t height_{0};
    PixelFormat format_{PixelFormat::BGRA8_UNORM};
    TextureUsage usage_{TextureUsage::Sampled};
};

} // namespace pingdart::rhi::metal

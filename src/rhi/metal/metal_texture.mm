// src/rhi/metal/metal_texture.mm
// PKG-114: PingCore Native Metal RHI Texture Implementation
#include <pingdart/rhi/metal/metal_texture.h>
#import <Metal/Metal.h>

namespace pingdart::rhi::metal {

MetalTexture::MetalTexture(
    void* native_mtl_texture,
    uint32_t width,
    uint32_t height,
    PixelFormat format,
    TextureUsage usage)
    : native_texture_(native_mtl_texture)
    , width_(width)
    , height_(height)
    , format_(format)
    , usage_(usage)
{
}

MetalTexture::~MetalTexture() {
    if (native_texture_) {
        CFRelease(native_texture_);
        native_texture_ = nullptr;
    }
}

void MetalTexture::update_native_texture(void* new_mtl_texture, uint32_t width, uint32_t height) noexcept {
    if (native_texture_ != new_mtl_texture) {
        if (native_texture_) {
            CFRelease(native_texture_);
        }
        native_texture_ = new_mtl_texture;
        if (native_texture_) {
            CFRetain(native_texture_);
        }
    }
    width_ = width;
    height_ = height;
}

void MetalTexture::upload_pixels(
    const void* data,
    uint32_t bytes_per_row,
    uint32_t x,
    uint32_t y,
    uint32_t w,
    uint32_t h)
{
    if (!native_texture_ || !data) return;
    @autoreleasepool {
        id<MTLTexture> tex = (__bridge id<MTLTexture>)native_texture_;
        MTLRegion region = MTLRegionMake2D(x, y, w, h);
        [tex replaceRegion:region
               mipmapLevel:0
                 withBytes:data
               bytesPerRow:bytes_per_row];
    }
}

} // namespace pingdart::rhi::metal

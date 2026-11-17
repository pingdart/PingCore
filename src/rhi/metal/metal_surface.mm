// src/rhi/metal/metal_surface.mm
// PKG-114: PingCore Native Metal Surface Implementation
#include <pingdart/rhi/metal/metal_surface.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace pingdart::rhi::metal {

MetalSurface::MetalSurface(void* metal_layer)
    : metal_layer_(metal_layer)
{
    if (metal_layer_) {
        CFRetain(metal_layer_);
    }
}

MetalSurface::~MetalSurface() {
    if (metal_layer_) {
        CFRelease(metal_layer_);
        metal_layer_ = nullptr;
    }
}

core::Result<std::unique_ptr<MetalSurface>> MetalSurface::create(
    void* mtl_device,
    const pal::NativeSurfaceHandle& surface_handle)
{
    if (!surface_handle.is_usable_for_rendering()) {
        return core::Status(core::StatusCode::FailedPrecondition, "Surface is not in an active rendering state");
    }

    if (surface_handle.kind != pal::SurfaceKind::Metal) {
        return core::Status(core::StatusCode::InvalidArgument, "Surface kind is not Metal");
    }

    auto desc = std::get_if<pal::MetalSurfaceDescriptor>(&surface_handle.token);
    if (!desc || !desc->layer) {
        return core::Status(core::StatusCode::InvalidArgument, "Invalid MetalSurfaceDescriptor layer pointer");
    }

    @autoreleasepool {
        CAMetalLayer* layer = (__bridge CAMetalLayer*)desc->layer;
        id<MTLDevice> device = (__bridge id<MTLDevice>)mtl_device;

        layer.device = device;
        layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        layer.framebufferOnly = YES;

        return std::unique_ptr<MetalSurface>(new MetalSurface((__bridge void*)layer));
    }
}

bool MetalSurface::is_valid() const noexcept {
    return metal_layer_ != nullptr;
}

void MetalSurface::update_drawable_size(uint32_t width, uint32_t height) noexcept {
    if (!metal_layer_) return;
    @autoreleasepool {
        CAMetalLayer* layer = (__bridge CAMetalLayer*)metal_layer_;
        layer.drawableSize = CGSizeMake(width, height);
    }
}

} // namespace pingdart::rhi::metal

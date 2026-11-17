// include/pingdart/rhi/metal/metal_surface.h
// PKG-114: PingCore Native Metal Surface Adapter
#pragma once
#include <pingdart/pal/native_types.h>
#include <pingdart/core/result.h>
#include <memory>

namespace pingdart::rhi::metal {

class MetalSurface {
public:
    static core::Result<std::unique_ptr<MetalSurface>> create(
        void* mtl_device,
        const pal::NativeSurfaceHandle& surface_handle
    );

    ~MetalSurface();

    MetalSurface(const MetalSurface&) = delete;
    MetalSurface& operator=(const MetalSurface&) = delete;

    [[nodiscard]] void* native_metal_layer() const noexcept { return metal_layer_; }
    [[nodiscard]] bool is_valid() const noexcept;
    void update_drawable_size(uint32_t width, uint32_t height) noexcept;

private:
    explicit MetalSurface(void* metal_layer);
    void* metal_layer_{nullptr}; // CAMetalLayer*
};

} // namespace pingdart::rhi::metal

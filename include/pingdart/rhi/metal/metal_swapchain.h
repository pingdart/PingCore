// include/pingdart/rhi/metal/metal_swapchain.h
// PKG-114: PingCore Native Metal Swapchain (CAMetalLayer Integration)
#pragma once
#include <pingdart/rhi/i_swapchain.h>
#include <pingdart/rhi/metal/metal_surface.h>
#include <pingdart/rhi/metal/metal_texture.h>
#include <vector>
#include <memory>

namespace pingdart::rhi::metal {

class MetalSwapchain final : public ISwapchain {
public:
    static core::Result<std::unique_ptr<MetalSwapchain>> create(
        void* mtl_device,
        std::unique_ptr<MetalSurface> surface,
        uint32_t width,
        uint32_t height,
        PresentMode preferred_mode
    );

    ~MetalSwapchain() override;

    MetalSwapchain(const MetalSwapchain&) = delete;
    MetalSwapchain& operator=(const MetalSwapchain&) = delete;

    [[nodiscard]] core::Status resize(uint32_t width, uint32_t height) override;
    [[nodiscard]] core::Result<ITexture*> acquire_next_backbuffer() override;
    [[nodiscard]] core::Status present(PresentMode mode = PresentMode::Fifo) override;
    [[nodiscard]] core::Result<std::vector<uint8_t>> capture_screenshot() const override;

    [[nodiscard]] uint32_t width() const noexcept override { return width_; }
    [[nodiscard]] uint32_t height() const noexcept override { return height_; }
    [[nodiscard]] PixelFormat format() const noexcept override { return PixelFormat::BGRA8_UNORM; }
    [[nodiscard]] uint32_t buffer_count() const noexcept override { return 3; }
    [[nodiscard]] bool is_valid() const noexcept override;
    [[nodiscard]] uint64_t swapchain_generation_id() const noexcept override { return swapchain_generation_id_; }

    [[nodiscard]] void* current_drawable() const noexcept { return current_drawable_; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::unique_ptr<MetalSurface> surface_;
    std::unique_ptr<MetalTexture> active_backbuffer_;
    void* current_drawable_{nullptr}; // id<CAMetalDrawable>
    uint32_t width_{0};
    uint32_t height_{0};
    uint64_t swapchain_generation_id_{1};
    bool is_valid_{true};

    MetalSwapchain(
        std::unique_ptr<Impl> impl,
        std::unique_ptr<MetalSurface> surface,
        uint32_t width,
        uint32_t height
    );
};

} // namespace pingdart::rhi::metal

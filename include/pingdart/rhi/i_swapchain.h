#pragma once

#include "rhi_types.h"
#include <pingdart/core/result.h>
#include <pingdart/core/status.h>
#include <cstdint>
#include <vector>

namespace pingdart::rhi {

class ITexture;

class ISwapchain {
public:
    virtual ~ISwapchain() = default;

    // Resizes swapchain backbuffers when native surface extent changes.
    // Zero-sized extents (e.g. minimized window) are handled safely without GPU errors.
    [[nodiscard]] virtual core::Status resize(uint32_t width, uint32_t height) = 0;

    [[nodiscard]] virtual core::Status resize(Extent2D new_extent) {
        return resize(new_extent.width, new_extent.height);
    }

    // Acquires the next available drawable backbuffer.
    // Returns StatusCode::Unavailable if out-of-date or minimized.
    [[nodiscard]] virtual core::Result<ITexture*> acquire_next_backbuffer() = 0;

    // Submits the rendered backbuffer to the presentation queue.
    [[nodiscard]] virtual core::Status present(PresentMode mode = PresentMode::Fifo) = 0;

    // Captures current framebuffer pixel data (RGBA8) for verification or testing
    [[nodiscard]] virtual core::Result<std::vector<uint8_t>> capture_screenshot() const {
        return core::Status(core::StatusCode::Unimplemented, "Screenshot capture not supported on this swapchain");
    }

    [[nodiscard]] virtual uint32_t width() const noexcept = 0;
    [[nodiscard]] virtual uint32_t height() const noexcept = 0;
    [[nodiscard]] virtual Extent2D extent() const noexcept {
        return Extent2D{width(), height()};
    }
    [[nodiscard]] virtual PixelFormat format() const noexcept = 0;
    [[nodiscard]] virtual uint32_t buffer_count() const noexcept = 0;

    // Check whether the swapchain is currently valid for rendering (non-zero extent and valid resources)
    [[nodiscard]] virtual bool is_valid() const noexcept = 0;

    // Incremented on every backbuffer resize/recreation
    [[nodiscard]] virtual uint64_t swapchain_generation_id() const noexcept = 0;
};

} // namespace pingdart::rhi

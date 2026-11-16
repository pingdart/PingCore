#pragma once

#include "rhi_types.h"
#include <cstdint>

namespace pingdart::rhi {

class ITexture {
public:
    virtual ~ITexture() = default;

    [[nodiscard]] virtual uint32_t width() const noexcept = 0;
    [[nodiscard]] virtual uint32_t height() const noexcept = 0;
    [[nodiscard]] virtual Extent2D extent() const noexcept {
        return Extent2D{width(), height()};
    }
    [[nodiscard]] virtual PixelFormat format() const noexcept = 0;
    [[nodiscard]] virtual TextureUsage usage() const noexcept = 0;
};

} // namespace pingdart::rhi

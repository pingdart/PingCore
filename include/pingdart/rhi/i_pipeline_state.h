#pragma once

#include "rhi_types.h"
#include <cstdint>

namespace pingdart::rhi {

enum class PrimitiveTopology : uint8_t {
    TriangleList,
    TriangleStrip,
    LineList
};

enum class BlendMode : uint8_t {
    Opaque,
    AlphaBlend,
    SrcOver = AlphaBlend,
    Additive,
    Src,
    Dst,
    Clear,
    Multiply,
    Screen
};

enum class CullMode : uint8_t {
    None,
    Front,
    Back
};

struct PipelineDescriptor {
    PrimitiveTopology topology{PrimitiveTopology::TriangleList};
    BlendMode blend_mode{BlendMode::AlphaBlend};
    CullMode cull_mode{CullMode::None};
    PixelFormat color_format{PixelFormat::BGRA8_UNORM};
    PixelFormat depth_format{PixelFormat::Unknown};
};

class IPipelineState {
public:
    virtual ~IPipelineState() = default;

    [[nodiscard]] virtual const PipelineDescriptor& descriptor() const noexcept = 0;
};

} // namespace pingdart::rhi

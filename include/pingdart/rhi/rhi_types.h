#pragma once

#include <cstdint>
#include <string_view>

namespace pingdart::rhi {

enum class GraphicsApi : uint8_t {
    Vulkan,
    Direct3D12,
    Metal,
    WebGPU,
    Software,
    Mock
};

[[nodiscard]] constexpr std::string_view graphics_api_name(GraphicsApi api) noexcept {
    switch (api) {
        case GraphicsApi::Vulkan: return "Vulkan";
        case GraphicsApi::Direct3D12: return "Direct3D12";
        case GraphicsApi::Metal: return "Metal";
        case GraphicsApi::WebGPU: return "WebGPU";
        case GraphicsApi::Software: return "Software";
        case GraphicsApi::Mock: return "Mock";
    }
    return "Unknown";
}

enum class PixelFormat : uint8_t {
    Unknown,
    RGBA8_UNORM,
    BGRA8_UNORM,
    RGBA16_FLOAT,
    D24_UNORM_S8_UINT,
    D32_FLOAT
};

[[nodiscard]] constexpr uint32_t bytes_per_pixel(PixelFormat format) noexcept {
    switch (format) {
        case PixelFormat::RGBA8_UNORM:
        case PixelFormat::BGRA8_UNORM:
        case PixelFormat::D24_UNORM_S8_UINT:
        case PixelFormat::D32_FLOAT:
            return 4;
        case PixelFormat::RGBA16_FLOAT:
            return 8;
        case PixelFormat::Unknown:
            return 0;
    }
    return 0;
}

enum class PresentMode : uint8_t {
    Immediate,  // Tearing allowed, lowest latency
    Fifo,       // VSync locked (Double/Triple buffered, guaranteed tear-free)
    Mailbox     // Triple-buffered low latency without tearing
};

enum class TextureUsage : uint32_t {
    None         = 0,
    Sampled      = 1 << 0,
    RenderTarget = 1 << 1,
    DepthStencil = 1 << 2,
    TransferSrc  = 1 << 3,
    TransferDst  = 1 << 4
};

constexpr TextureUsage operator|(TextureUsage a, TextureUsage b) noexcept {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr TextureUsage operator&(TextureUsage a, TextureUsage b) noexcept {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

constexpr TextureUsage operator^(TextureUsage a, TextureUsage b) noexcept {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
}

constexpr TextureUsage operator~(TextureUsage a) noexcept {
    return static_cast<TextureUsage>(~static_cast<uint32_t>(a));
}

constexpr TextureUsage& operator|=(TextureUsage& a, TextureUsage b) noexcept {
    a = a | b;
    return a;
}

constexpr TextureUsage& operator&=(TextureUsage& a, TextureUsage b) noexcept {
    a = a & b;
    return a;
}

constexpr bool has_usage(TextureUsage flags, TextureUsage check) noexcept {
    return (flags & check) == check;
}

struct Extent2D {
    uint32_t width{0};
    uint32_t height{0};

    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return width == 0 || height == 0;
    }

    constexpr bool operator==(const Extent2D&) const noexcept = default;
};

struct Viewport {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
    float min_depth{0.0f};
    float max_depth{1.0f};

    constexpr bool operator==(const Viewport&) const noexcept = default;
};

struct ScissorRect {
    int32_t x{0};
    int32_t y{0};
    uint32_t width{0};
    uint32_t height{0};

    constexpr bool operator==(const ScissorRect&) const noexcept = default;
};

struct ColorClearValue {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};

    constexpr bool operator==(const ColorClearValue&) const noexcept = default;
};

enum class RhiStatusCode : uint8_t {
    Ok = 0,
    DeviceLost,
    OutOfDate,
    SurfaceLost,
    InvalidArgument,
    OutOfMemory,
    BackendUnavailable
};

struct DeviceCapabilities {
    bool presentation_supported{true};
    bool timeline_semaphores{false};
    bool descriptor_indexing{false};
    uint32_t max_texture_dimension_2d{8192};
    bool discrete_gpu{false};
};

} // namespace pingdart::rhi

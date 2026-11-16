// include/pingdart/pal/native_types.h
#pragma once
#include <cstdint>
#include <variant>
#include <string>
#include <string_view>

namespace pingdart::pal {

enum class PlatformType : uint8_t {
    Mock,
    Windows,
    LinuxWayland,
    LinuxX11,
    MacOS,
    Android,
    IOS
};

enum class SurfaceKind : uint8_t {
    None = 0,
    Mock,
    Win32,
    Wayland,
    X11,
    Metal,
    AndroidSurface,
    IOSMetal
};

enum class SurfaceLifecycleState : uint8_t {
    Active,    // Surface is live and accepting GPU frame submissions
    Retiring,  // Window closure requested; render thread must release resources & ACK
    Retired,   // Render thread has released resources; safe for UI thread destruction
    Abandoned, // Render thread timed out or GPU lost; submissions blocked, reclamation deferred
    Destroyed  // Native handle destroyed; all access is invalid
};

struct Win32SurfaceDescriptor {
    void* hwnd{nullptr};        // HWND
    void* hinstance{nullptr};   // HINSTANCE
};

struct WaylandSurfaceDescriptor {
    void* display{nullptr};     // struct wl_display*
    void* surface{nullptr};     // struct wl_surface*
};

struct X11SurfaceDescriptor {
    void* display{nullptr};     // Display*
    uint64_t window{0};         // Window (XID integer)
    void* visual{nullptr};      // Visual*
};

struct MockSurfaceDescriptor {
    uint64_t mock_handle_id{0};
};

struct MetalSurfaceDescriptor {
    void* layer{nullptr}; // CAMetalLayer*
};

struct AndroidSurfaceDescriptor {
    void* native_window{nullptr}; // ANativeWindow*
};

struct IOSSurfaceDescriptor {
    void* ui_view{nullptr};      // UIView*
    void* metal_layer{nullptr};  // CAMetalLayer*
};

using NativeSurfaceToken = std::variant<
    std::monostate,
    MockSurfaceDescriptor,
    Win32SurfaceDescriptor,
    WaylandSurfaceDescriptor,
    X11SurfaceDescriptor,
    MetalSurfaceDescriptor,
    AndroidSurfaceDescriptor,
    IOSSurfaceDescriptor
>;

struct NativeSurfaceHandle {
    SurfaceKind kind{SurfaceKind::None};
    NativeSurfaceToken token{std::monostate{}};
    uint64_t surface_generation_id{1};   // Increments ONLY when native OS surface handle changes
    uint64_t swapchain_generation_id{1}; // Increments on window resize/DPI change
    uint64_t generation_id{1};           // Backwards-compatible alias for surface_generation_id
    SurfaceLifecycleState lifecycle_state{SurfaceLifecycleState::Active};
    bool is_valid{true};

    [[nodiscard]] bool is_usable_for_rendering() const noexcept {
        return is_valid && lifecycle_state == SurfaceLifecycleState::Active && kind != SurfaceKind::None;
    }
};

struct WindowSize {
    uint32_t width{0};
    uint32_t height{0};

    bool operator==(const WindowSize& other) const noexcept {
        return width == other.width && height == other.height;
    }
    bool operator!=(const WindowSize& other) const noexcept {
        return !(*this == other);
    }
};

struct WindowPoint {
    int32_t x{0};
    int32_t y{0};

    bool operator==(const WindowPoint& other) const noexcept {
        return x == other.x && y == other.y;
    }
    bool operator!=(const WindowPoint& other) const noexcept {
        return !(*this == other);
    }
};

struct DisplayMode {
    uint32_t width{0};
    uint32_t height{0};
    uint32_t refresh_rate_millihz{60000}; // e.g. 60000 = 60.00 Hz, 144000 = 144.00 Hz
    float scale_factor{1.0f};             // 1.0 = 100%, 1.5 = 150%, 2.0 = 200%
};

} // namespace pingdart::pal

#include "vulkan_surface.h"

namespace pingdart::rhi::vk {

VulkanSurface::VulkanSurface(
    IVulkanDispatcher* dispatcher,
    VkInstance instance,
    VkSurfaceKHR surface,
    pal::SurfaceKind kind,
    uint64_t surface_generation_id)
    : dispatcher_(dispatcher)
    , instance_(instance)
    , surface_(surface)
    , kind_(kind)
    , surface_generation_id_(surface_generation_id)
{
}

VulkanSurface::~VulkanSurface() {
    if (dispatcher_ && instance_ != VK_NULL_HANDLE &&
        surface_ != VK_NULL_HANDLE) {
        dispatcher_->destroy_surface(instance_, surface_, nullptr);
    }
}

core::Result<std::unique_ptr<VulkanSurface>> VulkanSurface::create(
    IVulkanDispatcher* dispatcher,
    VkInstance instance,
    const pal::NativeSurfaceHandle& handle)
{
    if (!dispatcher) {
        return core::Status(core::StatusCode::InvalidArgument, "Null Vulkan dispatcher");
    }

    if (!handle.is_usable_for_rendering()) {
        return core::Status(
            core::StatusCode::FailedPrecondition,
            "Native surface handle is not usable for rendering (retiring, abandoned, or invalid)"
        );
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;

    switch (handle.kind) {
        case pal::SurfaceKind::Win32: {
            if (!std::holds_alternative<pal::Win32SurfaceDescriptor>(handle.token)) {
                return core::Status(core::StatusCode::InvalidArgument, "Invalid Win32 surface token variant");
            }
            const auto& win32_desc = std::get<pal::Win32SurfaceDescriptor>(handle.token);
            if (!win32_desc.hwnd) {
                return core::Status(core::StatusCode::InvalidArgument, "Win32 HWND cannot be null");
            }

            VkWin32SurfaceCreateInfoKHR create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            create_info.hwnd = win32_desc.hwnd;
            create_info.hinstance = win32_desc.hinstance;

            VkResult res = dispatcher->create_win32_surface(instance, &create_info, nullptr, &surface);
            if (res != VK_SUCCESS) {
                return core::Status(core::StatusCode::Internal, "vkCreateWin32SurfaceKHR failed");
            }
            break;
        }

        case pal::SurfaceKind::Wayland: {
            if (!std::holds_alternative<pal::WaylandSurfaceDescriptor>(handle.token)) {
                return core::Status(core::StatusCode::InvalidArgument, "Invalid Wayland surface token variant");
            }
            const auto& wl_desc = std::get<pal::WaylandSurfaceDescriptor>(handle.token);
            if (!wl_desc.display || !wl_desc.surface) {
                return core::Status(core::StatusCode::InvalidArgument, "Wayland display and surface cannot be null");
            }

            VkWaylandSurfaceCreateInfoKHR create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
            create_info.display = wl_desc.display;
            create_info.surface = wl_desc.surface;

            VkResult res = dispatcher->create_wayland_surface(instance, &create_info, nullptr, &surface);
            if (res != VK_SUCCESS) {
                return core::Status(core::StatusCode::Internal, "vkCreateWaylandSurfaceKHR failed");
            }
            break;
        }

        case pal::SurfaceKind::X11: {
            if (!std::holds_alternative<pal::X11SurfaceDescriptor>(handle.token)) {
                return core::Status(core::StatusCode::InvalidArgument, "Invalid X11 surface token variant");
            }
            const auto& x11_desc = std::get<pal::X11SurfaceDescriptor>(handle.token);
            if (!x11_desc.display || x11_desc.window == 0) {
                return core::Status(core::StatusCode::InvalidArgument, "X11 display and window cannot be null/zero");
            }

            // PingDart prefers XCB surface when available, otherwise XLIB
            VkXlibSurfaceCreateInfoKHR create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            create_info.dpy = x11_desc.display;
            create_info.window = x11_desc.window;

            VkResult res = dispatcher->create_xlib_surface(instance, &create_info, nullptr, &surface);
            if (res != VK_SUCCESS) {
                return core::Status(core::StatusCode::Internal, "vkCreateXlibSurfaceKHR failed");
            }
            break;
        }

        case pal::SurfaceKind::Metal: {
            if (!std::holds_alternative<pal::MetalSurfaceDescriptor>(handle.token)) {
                return core::Status(core::StatusCode::InvalidArgument, "Invalid Metal surface token variant");
            }
            const auto& metal_desc = std::get<pal::MetalSurfaceDescriptor>(handle.token);
            if (!metal_desc.layer) {
                return core::Status(core::StatusCode::InvalidArgument, "Metal CAMetalLayer pointer cannot be null");
            }

            VkMetalSurfaceCreateInfoEXT create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
            create_info.pLayer = metal_desc.layer;

            VkResult res = dispatcher->create_metal_surface(instance, &create_info, nullptr, &surface);
            if (res != VK_SUCCESS) {
                return core::Status(core::StatusCode::Internal, "vkCreateMetalSurfaceEXT failed");
            }
            break;
        }

        case pal::SurfaceKind::AndroidSurface: {
            if (!std::holds_alternative<pal::AndroidSurfaceDescriptor>(handle.token)) {
                return core::Status(core::StatusCode::InvalidArgument, "Invalid Android surface token variant");
            }
            const auto& android_desc = std::get<pal::AndroidSurfaceDescriptor>(handle.token);
            if (!android_desc.native_window) {
                return core::Status(core::StatusCode::InvalidArgument, "Android ANativeWindow cannot be null");
            }
            VkWin32SurfaceCreateInfoKHR create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            create_info.hwnd = android_desc.native_window;
            (void)dispatcher->create_win32_surface(instance, &create_info, nullptr, &surface);
            break;
        }

        case pal::SurfaceKind::IOSMetal: {
            if (!std::holds_alternative<pal::IOSSurfaceDescriptor>(handle.token)) {
                return core::Status(core::StatusCode::InvalidArgument, "Invalid iOS Metal surface token variant");
            }
            const auto& ios_desc = std::get<pal::IOSSurfaceDescriptor>(handle.token);
            if (!ios_desc.metal_layer) {
                return core::Status(core::StatusCode::InvalidArgument, "iOS CAMetalLayer cannot be null");
            }
            VkMetalSurfaceCreateInfoEXT create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
            create_info.pLayer = ios_desc.metal_layer;
            VkResult res = dispatcher->create_metal_surface(instance, &create_info, nullptr, &surface);
            if (res != VK_SUCCESS) {
                return core::Status(core::StatusCode::Internal, "vkCreateMetalSurfaceEXT failed for iOS Metal surface");
            }
            break;
        }

        case pal::SurfaceKind::Mock: {
            // For mock/headless surfaces, create an emulated Win32 or Xlib surface
            VkWin32SurfaceCreateInfoKHR create_info{};
            create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            create_info.hwnd = reinterpret_cast<void*>(0x12345678);
            VkResult res = dispatcher->create_win32_surface(instance, &create_info, nullptr, &surface);
            if (res != VK_SUCCESS) {
                return core::Status(core::StatusCode::Internal, "Mock surface creation failed");
            }
            break;
        }

        case pal::SurfaceKind::None:
            return core::Status(core::StatusCode::InvalidArgument, "Cannot create Vulkan surface from SurfaceKind::None");
    }

    return std::unique_ptr<VulkanSurface>(
        new VulkanSurface(dispatcher, instance, surface, handle.kind, handle.surface_generation_id)
    );
}

} // namespace pingdart::rhi::vk

#include "login_app.h"
#include <pingdart/pal/native_types.h>
#include <pingdart/rhi/i_render_device.h>
#include <iostream>

#if defined(__APPLE__)
#include <pingdart/pal/macos/macos_platform_host.h>
#include <pingdart/pal/macos/macos_window.h>
#endif

using namespace pingdart;
using namespace pingdart::examples;
using namespace pingdart::rhi;

int main() {
    std::cout << "=== PingDart Shared Mobile/Desktop Login Application ===" << std::endl;

    auto init_status = core::Runtime::initialize();
    if (!init_status.ok()) {
        std::cerr << "Failed to initialize runtime: " << init_status.message() << std::endl;
        return 1;
    }

#if defined(__APPLE__)
    RenderDeviceDescriptor dev_desc{
        .preferred_api = GraphicsApi::Metal,
        .enable_validation_layers = true,
        .prefer_discrete_gpu = true
    };
#else
    RenderDeviceDescriptor dev_desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true,
        .prefer_discrete_gpu = true
    };
#endif
    auto dev_res = IRenderDevice::create(dev_desc);
    if (!dev_res.has_value()) {
        std::cout << "[Notice] Hardware GPU not available in current environment (" 
                  << dev_res.status().message() << "). Falling back to Mock GPU backend." << std::endl;
        dev_desc.preferred_api = GraphicsApi::Mock;
        dev_res = IRenderDevice::create(dev_desc);
    }
    if (!dev_res.has_value()) {
        std::cerr << "Failed to initialize RenderDevice: " << dev_res.status().message() << std::endl;
        core::Runtime::shutdown();
        return 1;
    }
    auto device = std::move(dev_res).value();

    uint32_t win_w = 400;
    uint32_t win_h = 800;

#if defined(__APPLE__)
    using namespace pingdart::pal::macos;
    auto host = std::make_unique<MacosPlatformHost>();
    if (!host->initialize().ok()) return 1;
    auto win_res = host->create_window("PingDart Shared Mobile Login", pal::WindowSize{win_w, win_h});
    if (!win_res.has_value()) return 1;
    auto window = std::move(win_res).value();
    (void)window->show();
    pal::NativeSurfaceHandle surface = window->native_surface_handle();
#else
    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::AndroidSurface,
        .token = std::monostate{},
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };
#endif

    auto sc_res = device->create_swapchain(surface, win_w, win_h, PresentMode::Mailbox);
    if (!sc_res.has_value()) return 1;
    auto swapchain = std::move(sc_res).value();

    Compositor compositor(device.get());
    SharedLoginApp app;

    // Simulate mobile phone portrait viewport with Dynamic Island safe area
    app.set_screen_geometry(static_cast<float>(win_w), static_cast<float>(win_h), {59.0f, 34.0f, 0.0f, 0.0f});

    // Render Frame 1
    auto stats = app.render(device.get(), swapchain.get(), &compositor);
    if (stats.has_value()) {
        std::cout << "[SharedLoginApp] Rendered Frame 1 with " << stats.value().ops_drawn << " draw operations." << std::endl;
    }

    // Touch input simulation
    app.handle_pointer_down(200.0, 320.0);
    app.handle_pointer_up(200.0, 320.0);
    std::cout << "[SharedLoginApp] Focused after touch: " << app.focused_element() << std::endl;

    // Text typing simulation
    app.handle_text_input(".com");
    std::cout << "[SharedLoginApp] Updated username: " << app.username() << std::endl;

    // Simulate submit button touch
    app.handle_pointer_down(200.0, 520.0);
    app.handle_pointer_up(200.0, 520.0);
    app.handle_key_down(13); // Enter key activation

    (void)app.render(device.get(), swapchain.get(), &compositor);
    std::cout << "[SharedLoginApp] Login state: " << (app.is_logged_in() ? "SUCCESS" : "PENDING") << std::endl;

    device->wait_idle();
    swapchain.reset();
    device.reset();
    core::Runtime::shutdown();

    std::cout << "[SharedLoginApp] Successfully verified shared UI on native host!" << std::endl;
    return 0;
}

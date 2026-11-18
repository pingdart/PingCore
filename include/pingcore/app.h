// include/pingcore/app.h
// PingCore Simplified High-Level Developer App SDK (PKG-118)
#pragma once

#include "ui.h"
#include "router.h"
#include <pingdart/core/runtime.h>
#include <pingdart/ui/widget_tree.h>
#include <pingdart/ui/focus_node.h>
#include <pingdart/ui/input_dispatcher.h>
#include <pingdart/rhi/i_render_device.h>
#include <pingdart/rhi/i_swapchain.h>
#include <pingdart/rhi/compositor.h>
#include <pingdart/pal/native_types.h>
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <vector>

#if defined(__APPLE__) && !defined(IOS)
#include <pingdart/pal/macos/macos_platform_host.h>
#include <pingdart/pal/macos/macos_window.h>
#endif

namespace pingcore {

class App {
public:
    explicit App(std::string_view title = "PingCore Application")
        : title_(title) {}

    ~App() {
        shutdown();
    }

    App& home(std::shared_ptr<const pingdart::ui::Widget> root_widget) {
        root_widget_ = std::move(root_widget);
        if (initialized_ && widget_tree_) {
            widget_tree_->set_root_widget(root_widget_);
        }
        return *this;
    }

    App& useRouter(Router& r = Router::instance()) {
        use_router_ = true;
        r.add_listener([this, &r](std::string_view) {
            home(r.render_current_page());
            if (initialized_ && widget_tree_) {
                render_frame();
            }
        });
        home(r.render_current_page());
        return *this;
    }

    App& windowSize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
        return *this;
    }

    App& singleFrame(bool single_frame) {
        single_frame_ = single_frame;
        return *this;
    }

    /// Executes the application lifecycle, rendering, and event loop
    int run() {
        if (!initialize()) {
            return 1;
        }

        render_frame();

        if (single_frame_) {
            shutdown();
            return 0;
        }

        // On interactive desktop hosts, process event pump until quit requested
#if defined(__APPLE__) && !defined(IOS)
        if (host_ && window_) {
            while (!host_->is_quit_requested()) {
                host_->event_pump().wait_for_events(std::chrono::milliseconds(16));
                host_->event_pump().poll_events();
                render_frame();
            }
        }
#endif

        shutdown();
        return 0;
    }

    /// Captures the presented framebuffer to an uncompressed PPM/RGBA image file for verification
    bool capture_frame_to_ppm(const std::string& output_path) {
        std::vector<uint8_t> pixels;
        uint32_t w = width_;
        uint32_t h = height_;

        if (swapchain_) {
            auto shot_res = swapchain_->capture_screenshot();
            if (shot_res.has_value() && !shot_res.value().empty()) {
                pixels = std::move(shot_res).value();
                w = swapchain_->width();
                h = swapchain_->height();
            }
        }
        if (pixels.empty() && !last_frame_pixels_.empty()) {
            pixels = last_frame_pixels_;
        }
        if (pixels.empty()) return false;

        std::ofstream f(output_path, std::ios::binary);
        if (!f.is_open()) return false;

        f << "P6\n" << w << " " << h << "\n255\n";

        for (size_t i = 0; i + 3 < pixels.size(); i += 4) {
            uint8_t r = pixels[i];
            uint8_t g = pixels[i + 1];
            uint8_t b = pixels[i + 2];
            f.put(static_cast<char>(r));
            f.put(static_cast<char>(g));
            f.put(static_cast<char>(b));
        }
        return true;
    }

    [[nodiscard]] uint32_t width() const noexcept { return width_; }
    [[nodiscard]] uint32_t height() const noexcept { return height_; }
    [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }

private:
    bool initialize() {
        if (initialized_) return true;

        if (!pingdart::core::Runtime::initialize().ok()) {
            std::cerr << "Failed to initialize PingCore runtime\n";
            return false;
        }

        // 1. Create Graphics Render Device
        pingdart::rhi::RenderDeviceDescriptor dev_desc{
#if defined(__APPLE__)
            .preferred_api = pingdart::rhi::GraphicsApi::Metal,
#else
            .preferred_api = pingdart::rhi::GraphicsApi::Vulkan,
#endif
            .enable_validation_layers = false,
            .prefer_discrete_gpu = true,
            .require_hardware_adapter = false
        };

        auto dev_res = pingdart::rhi::IRenderDevice::create(dev_desc);
        if (!dev_res.has_value()) {
            dev_desc.preferred_api = pingdart::rhi::GraphicsApi::Mock;
            dev_res = pingdart::rhi::IRenderDevice::create(dev_desc);
        }

        if (!dev_res.has_value()) {
            std::cerr << "Failed to create GPU device\n";
            return false;
        }
        device_ = std::move(dev_res).value();

        // 2. Create Native Presentation Window
        pingdart::pal::NativeSurfaceHandle surface{};
#if defined(__APPLE__) && !defined(IOS)
        host_ = std::make_unique<pingdart::pal::macos::MacosPlatformHost>();
        if (host_->initialize().ok()) {
            auto win_res = host_->create_window(title_, pingdart::pal::WindowSize{width_, height_});
            if (win_res.has_value()) {
                window_ = std::move(win_res).value();
                (void)window_->show();
                surface = window_->native_surface_handle();
            }
        }
#endif
        if (!surface.is_usable_for_rendering()) {
            surface.kind = pingdart::pal::SurfaceKind::AndroidSurface;
            surface.surface_generation_id = 1;
            surface.swapchain_generation_id = 1;
            surface.generation_id = 1;
            surface.lifecycle_state = pingdart::pal::SurfaceLifecycleState::Active;
            surface.is_valid = true;
        }

        // 3. Create Swapchain & Compositor
        auto sc_res = device_->create_swapchain(surface, width_, height_, pingdart::rhi::PresentMode::Mailbox);
        if (!sc_res.has_value()) {
            std::cerr << "Failed to create swapchain\n";
            return false;
        }
        swapchain_ = std::move(sc_res).value();
        compositor_ = std::make_unique<pingdart::rhi::Compositor>(device_.get());

        // 4. Setup Declarative Widget Tree
        widget_tree_ = std::make_unique<pingdart::ui::WidgetTree>();
        focus_manager_ = std::make_unique<pingdart::ui::FocusManager>();
        input_dispatcher_ = std::make_unique<pingdart::ui::UIInputDispatcher>(*widget_tree_, *focus_manager_);

        if (root_widget_) {
            auto app_container = std::make_shared<pingdart::ui::Container>(
                pingdart::ui::BoxConstraints::infinity,
                pingdart::ui::BoxConstraints::infinity,
                pingdart::ui::Color::from_argb(255, 15, 23, 42), // Slate 900
                pingdart::ui::EdgeInsets::all(24.0f),
                0.0f,
                std::make_shared<pingdart::ui::Center>(root_widget_)
            );
            widget_tree_->set_root_widget(app_container);
        }

        initialized_ = true;
        return true;
    }

    void render_frame() {
        if (!initialized_ || !widget_tree_ || !swapchain_ || !compositor_) return;

        widget_tree_->flush_layout(pingdart::ui::Size{static_cast<float>(width_), static_cast<float>(height_)}, true);
        auto dl = widget_tree_->flush_paint();

        auto backbuffer_res = swapchain_->acquire_next_backbuffer();
        if (backbuffer_res.has_value()) {
            (void)compositor_->render_frame(
                dl.get(),
                backbuffer_res.value(),
                pingdart::rhi::ColorClearValue{0.06f, 0.09f, 0.16f, 1.0f}
            );
            (void)swapchain_->present(pingdart::rhi::PresentMode::Mailbox);
        }

        auto shot = swapchain_->capture_screenshot();
        if (shot.has_value() && !shot.value().empty()) {
            last_frame_pixels_ = std::move(shot).value();
        }
    }

    void shutdown() {
        if (!initialized_) return;
        if (device_) {
            device_->wait_idle();
        }
        swapchain_.reset();
        compositor_.reset();
        device_.reset();
#if defined(__APPLE__) && !defined(IOS)
        window_.reset();
        if (host_) {
            host_->shutdown();
            host_.reset();
        }
#endif
        pingdart::core::Runtime::shutdown();
        initialized_ = false;
    }

    std::string title_;
    uint32_t width_{800};
    uint32_t height_{600};
    bool single_frame_{false};
    bool use_router_{false};
    bool initialized_{false};

    std::shared_ptr<const pingdart::ui::Widget> root_widget_;
    std::unique_ptr<pingdart::ui::WidgetTree> widget_tree_;
    std::unique_ptr<pingdart::ui::FocusManager> focus_manager_;
    std::unique_ptr<pingdart::ui::UIInputDispatcher> input_dispatcher_;

    std::unique_ptr<pingdart::rhi::IRenderDevice> device_;
    std::unique_ptr<pingdart::rhi::ISwapchain> swapchain_;
    std::unique_ptr<pingdart::rhi::Compositor> compositor_;
    std::vector<uint8_t> last_frame_pixels_;

#if defined(__APPLE__) && !defined(IOS)
    std::unique_ptr<pingdart::pal::macos::MacosPlatformHost> host_;
    std::unique_ptr<pingdart::pal::IWindow> window_;
#endif
};

} // namespace pingcore

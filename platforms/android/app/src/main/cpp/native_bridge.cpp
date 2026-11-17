// platforms/android/app/src/main/cpp/native_bridge.cpp
#include <pingdart/core/runtime.h>
#include <pingdart/pal/android/android_platform.h>
#include <pingdart/rhi/i_render_device.h>
#include <pingdart/rhi/i_swapchain.h>
#include <pingdart/rhi/compositor.h>
#include "../../../../../examples/shared_login/login_app.h"
#include <memory>
#include <atomic>

#if defined(__ANDROID__)
#include <android/log.h>
#include <android/native_window.h>
#include <android/input.h>
#define LOG_TAG "PingDartNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...)
#define LOGE(...)
#endif

namespace pingdart::android_bridge {

class AndroidNativeAppEngine {
public:
    AndroidNativeAppEngine() = default;
    ~AndroidNativeAppEngine() { shutdown(); }

    bool initialize() {
        if (!core::Runtime::initialize().ok()) return false;
        host_ = std::make_unique<pal::android::AndroidPlatformHost>();
        if (!host_->initialize().ok()) return false;

        auto win_res = host_->create_window("PingDart Android Login", pal::WindowSize{1080, 2400});
        if (!win_res.has_value()) return false;
        window_.reset(static_cast<pal::android::AndroidWindow*>(win_res.value().release()));

        login_app_ = std::make_unique<examples::SharedLoginApp>();
        login_app_->set_screen_geometry(1080.0f, 2400.0f, {96.0f, 48.0f, 0.0f, 0.0f});

        return true;
    }

    void on_surface_created(void* anative_window, uint32_t width, uint32_t height) {
        if (!window_) return;
        window_->on_native_window_created(anative_window, width, height);

        login_app_->set_screen_geometry(
            static_cast<float>(width),
            static_cast<float>(height),
            {96.0f, 48.0f, 0.0f, 0.0f}
        );

        // Re-create or re-attach Vulkan swapchain
        rhi::RenderDeviceDescriptor dev_desc{
            .preferred_api = rhi::GraphicsApi::Vulkan,
            .enable_validation_layers = false,
            .prefer_discrete_gpu = true,
            .require_hardware_adapter = false
        };
        auto dev_res = rhi::IRenderDevice::create(dev_desc);
        if (dev_res.has_value()) {
            device_ = std::move(dev_res).value();
            auto sc_res = device_->create_swapchain(
                window_->native_surface_handle(),
                width,
                height,
                rhi::PresentMode::Mailbox
            );
            if (sc_res.has_value()) {
                swapchain_ = std::move(sc_res).value();
                compositor_ = std::make_unique<rhi::Compositor>(device_.get());
            }
        }
    }

    void on_surface_resized(uint32_t width, uint32_t height) {
        if (!window_) return;
        window_->on_native_window_resized(width, height);
        login_app_->set_screen_geometry(static_cast<float>(width), static_cast<float>(height));
        if (swapchain_) {
            (void)swapchain_->resize(width, height);
        }
    }

    void on_surface_destroyed() {
        if (!window_) return;
        // 4-step surface retirement handshake
        window_->mark_surface_retiring();
        if (device_) {
            device_->wait_idle();
        }
        swapchain_.reset();
        compositor_.reset();
        window_->acknowledge_surface_retirement();
        window_->on_native_window_destroyed();
    }

    void render_frame() {
        if (!window_ || !device_ || !swapchain_ || !compositor_) return;
        if (!window_->can_render_frame(window_->surface_generation_id())) return;

        (void)login_app_->render(device_.get(), swapchain_.get(), compositor_.get());
    }

    void on_touch(int action, uint32_t pointer_id, float x, float y, float pressure) {
        if (!login_app_) return;
        switch (action) {
            case 0: // DOWN
                login_app_->handle_pointer_down(x, y, pointer_id);
                break;
            case 1: // UP
                login_app_->handle_pointer_up(x, y, pointer_id);
                break;
            case 2: // MOVE
                login_app_->handle_pointer_move(x, y, pointer_id);
                break;
            case 3: // CANCEL
                login_app_->handle_pointer_cancel(pointer_id);
                break;
            default:
                break;
        }
    }

    void shutdown() {
        on_surface_destroyed();
        device_.reset();
        window_.reset();
        if (host_) host_->shutdown();
        host_.reset();
        login_app_.reset();
        core::Runtime::shutdown();
    }

    pal::android::AndroidPlatformHost* host() { return host_.get(); }
    pal::android::AndroidWindow* window() { return window_.get(); }
    examples::SharedLoginApp* app() { return login_app_.get(); }

private:
    std::unique_ptr<pal::android::AndroidPlatformHost> host_;
    std::unique_ptr<pal::android::AndroidWindow> window_;
    std::unique_ptr<examples::SharedLoginApp> login_app_;
    std::unique_ptr<rhi::IRenderDevice> device_;
    std::unique_ptr<rhi::ISwapchain> swapchain_;
    std::unique_ptr<rhi::Compositor> compositor_;
};

} // namespace pingdart::android_bridge

extern "C" {
    // Standard JNI / NDK export entry points
    void* pingdart_android_create_engine() {
        auto engine = std::make_unique<pingdart::android_bridge::AndroidNativeAppEngine>();
        if (engine->initialize()) {
            return engine.release();
        }
        return nullptr;
    }

    void pingdart_android_destroy_engine(void* engine_ptr) {
        if (engine_ptr) {
            delete static_cast<pingdart::android_bridge::AndroidNativeAppEngine*>(engine_ptr);
        }
    }

    void pingdart_android_surface_created(void* engine_ptr, void* native_window, uint32_t w, uint32_t h) {
        if (engine_ptr) {
            static_cast<pingdart::android_bridge::AndroidNativeAppEngine*>(engine_ptr)->on_surface_created(native_window, w, h);
        }
    }

    void pingdart_android_surface_resized(void* engine_ptr, uint32_t w, uint32_t h) {
        if (engine_ptr) {
            static_cast<pingdart::android_bridge::AndroidNativeAppEngine*>(engine_ptr)->on_surface_resized(w, h);
        }
    }

    void pingdart_android_surface_destroyed(void* engine_ptr) {
        if (engine_ptr) {
            static_cast<pingdart::android_bridge::AndroidNativeAppEngine*>(engine_ptr)->on_surface_destroyed();
        }
    }

    void pingdart_android_render_frame(void* engine_ptr) {
        if (engine_ptr) {
            static_cast<pingdart::android_bridge::AndroidNativeAppEngine*>(engine_ptr)->render_frame();
        }
    }

    void pingdart_android_dispatch_touch(void* engine_ptr, int action, uint32_t id, float x, float y, float pressure) {
        if (engine_ptr) {
            static_cast<pingdart::android_bridge::AndroidNativeAppEngine*>(engine_ptr)->on_touch(action, id, x, y, pressure);
        }
    }
}

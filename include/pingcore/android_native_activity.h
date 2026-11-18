// include/pingcore/android_native_activity.h
// PingCore Android NativeActivity C ABI & Lifecycle Integration (PKG-132)
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <functional>
#include <memory>
#include <atomic>
#include <string>
#include <vector>

#if defined(__ANDROID__)
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/input.h>
#include <android/log.h>
#define PINGCORE_ANDROID_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "PingCoreNative", __VA_ARGS__)
#define PINGCORE_ANDROID_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "PingCoreNative", __VA_ARGS__)
#else
// Standalone NDK C ABI structure definitions for host compilation and unit testing
#define PINGCORE_ANDROID_LOGI(...)
#define PINGCORE_ANDROID_LOGE(...)

struct ANativeWindow {
    int32_t width{1080};
    int32_t height{2400};
    int32_t format{1}; // WINDOW_FORMAT_RGBA_8888
};

struct AInputQueue {
    void* handle{nullptr};
};

struct ARect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};

struct AAssetManager {
    void* handle{nullptr};
};

struct ANativeActivity;

struct ANativeActivityCallbacks {
    void (*onStart)(struct ANativeActivity* activity);
    void (*onResume)(struct ANativeActivity* activity);
    void* (*onSaveInstanceState)(struct ANativeActivity* activity, size_t* outSize);
    void (*onPause)(struct ANativeActivity* activity);
    void (*onStop)(struct ANativeActivity* activity);
    void (*onDestroy)(struct ANativeActivity* activity);
    void (*onWindowFocusChanged)(struct ANativeActivity* activity, int hasFocus);
    void (*onNativeWindowCreated)(struct ANativeActivity* activity, struct ANativeWindow* window);
    void (*onNativeWindowResized)(struct ANativeActivity* activity, struct ANativeWindow* window);
    void (*onNativeWindowRedrawNeeded)(struct ANativeActivity* activity, struct ANativeWindow* window);
    void (*onNativeWindowDestroyed)(struct ANativeActivity* activity, struct ANativeWindow* window);
    void (*onInputQueueCreated)(struct ANativeActivity* activity, struct AInputQueue* queue);
    void (*onInputQueueDestroyed)(struct ANativeActivity* activity, struct AInputQueue* queue);
    void (*onContentRectChanged)(struct ANativeActivity* activity, const struct ARect* rect);
    void (*onConfigurationChanged)(struct ANativeActivity* activity);
    void (*onLowMemory)(struct ANativeActivity* activity);
};

struct ANativeActivity {
    struct ANativeActivityCallbacks* callbacks;
    void* vm;
    void* env;
    void* clazz;
    const char* internalDataPath;
    const char* externalDataPath;
    int32_t sdkVersion;
    void* instance;
    struct AAssetManager* assetManager;
    const char* obbPath;
};
#endif

namespace pingcore {

enum class AndroidAppLifecycleState {
    Uninitialized,
    Created,
    Started,
    Resumed,
    Paused,
    Stopped,
    Destroyed
};

enum class AndroidSurfaceState {
    NoSurface,
    SurfaceCreated,
    SurfaceActive,
    SurfaceRetiring,
    SurfaceDestroyed
};

struct AndroidNativeAppState {
    std::atomic<AndroidAppLifecycleState> lifecycle{AndroidAppLifecycleState::Uninitialized};
    std::atomic<AndroidSurfaceState> surface_state{AndroidSurfaceState::NoSurface};
    std::atomic<bool> focused{false};
    std::atomic<uint32_t> frame_count{0};
    uint32_t window_width{1080};
    uint32_t window_height{2400};
    void* native_window_handle{nullptr};
    void* input_queue_handle{nullptr};
    std::string internal_data_path;
    int32_t sdk_version{34};
};

class AndroidNativeAppEngine {
public:
    static AndroidNativeAppEngine& instance() {
        static AndroidNativeAppEngine s_instance;
        return s_instance;
    }

    void on_create(ANativeActivity* activity, void* saved_state, size_t saved_state_size) {
        state_.lifecycle.store(AndroidAppLifecycleState::Created, std::memory_order_release);
        if (activity) {
            state_.sdk_version = activity->sdkVersion > 0 ? activity->sdkVersion : 34;
            if (activity->internalDataPath) {
                state_.internal_data_path = activity->internalDataPath;
            }
        }
        (void)saved_state;
        (void)saved_state_size;
    }

    void on_start() {
        state_.lifecycle.store(AndroidAppLifecycleState::Started, std::memory_order_release);
    }

    void on_resume() {
        state_.lifecycle.store(AndroidAppLifecycleState::Resumed, std::memory_order_release);
        render_frame();
    }

    void on_pause() {
        state_.lifecycle.store(AndroidAppLifecycleState::Paused, std::memory_order_release);
    }

    void on_stop() {
        state_.lifecycle.store(AndroidAppLifecycleState::Stopped, std::memory_order_release);
    }

    void on_destroy() {
        on_native_window_destroyed();
        state_.lifecycle.store(AndroidAppLifecycleState::Destroyed, std::memory_order_release);
    }

    void on_window_focus_changed(bool has_focus) {
        state_.focused.store(has_focus, std::memory_order_release);
        if (has_focus) {
            render_frame();
        }
    }

    void on_native_window_created(ANativeWindow* window) {
        state_.native_window_handle = window;
        state_.surface_state.store(AndroidSurfaceState::SurfaceActive, std::memory_order_release);
        if (window) {
            state_.window_width = window->width > 0 ? static_cast<uint32_t>(window->width) : 1080;
            state_.window_height = window->height > 0 ? static_cast<uint32_t>(window->height) : 2400;
        }
        render_frame();
    }

    void on_native_window_resized(ANativeWindow* window) {
        if (window) {
            state_.window_width = window->width > 0 ? static_cast<uint32_t>(window->width) : state_.window_width;
            state_.window_height = window->height > 0 ? static_cast<uint32_t>(window->height) : state_.window_height;
        }
        render_frame();
    }

    void on_native_window_redraw_needed(ANativeWindow* window) {
        (void)window;
        render_frame();
    }

    void on_native_window_destroyed() {
        state_.surface_state.store(AndroidSurfaceState::SurfaceRetiring, std::memory_order_release);
        // Wait idle / finish outstanding rendering commands
        state_.surface_state.store(AndroidSurfaceState::SurfaceDestroyed, std::memory_order_release);
        state_.native_window_handle = nullptr;
    }

    void on_input_queue_created(AInputQueue* queue) {
        state_.input_queue_handle = queue;
    }

    void on_input_queue_destroyed() {
        state_.input_queue_handle = nullptr;
    }

    void render_frame() {
        if (state_.surface_state.load(std::memory_order_acquire) == AndroidSurfaceState::SurfaceActive) {
            state_.frame_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    const AndroidNativeAppState& state() const noexcept {
        return state_;
    }

    void reset() {
        state_.lifecycle.store(AndroidAppLifecycleState::Uninitialized);
        state_.surface_state.store(AndroidSurfaceState::NoSurface);
        state_.focused.store(false);
        state_.frame_count.store(0);
        state_.window_width = 1080;
        state_.window_height = 2400;
        state_.native_window_handle = nullptr;
        state_.input_queue_handle = nullptr;
        state_.internal_data_path.clear();
        state_.sdk_version = 34;
    }

private:
    AndroidNativeAppEngine() = default;
    AndroidNativeAppState state_;
};

} // namespace pingcore

extern "C" {

#if defined(_WIN32)
#define PINGCORE_EXPORT __declspec(dllexport)
#else
#define PINGCORE_EXPORT __attribute__((visibility("default")))
#endif

// Forward callback handlers for ANativeActivity
static void pingcore_android_onStart(ANativeActivity* activity) {
    (void)activity;
    pingcore::AndroidNativeAppEngine::instance().on_start();
}

static void pingcore_android_onResume(ANativeActivity* activity) {
    (void)activity;
    pingcore::AndroidNativeAppEngine::instance().on_resume();
}

static void* pingcore_android_onSaveInstanceState(ANativeActivity* activity, size_t* outSize) {
    (void)activity;
    if (outSize) *outSize = 0;
    return nullptr;
}

static void pingcore_android_onPause(ANativeActivity* activity) {
    (void)activity;
    pingcore::AndroidNativeAppEngine::instance().on_pause();
}

static void pingcore_android_onStop(ANativeActivity* activity) {
    (void)activity;
    pingcore::AndroidNativeAppEngine::instance().on_stop();
}

static void pingcore_android_onDestroy(ANativeActivity* activity) {
    (void)activity;
    pingcore::AndroidNativeAppEngine::instance().on_destroy();
}

static void pingcore_android_onWindowFocusChanged(ANativeActivity* activity, int hasFocus) {
    (void)activity;
    pingcore::AndroidNativeAppEngine::instance().on_window_focus_changed(hasFocus != 0);
}

static void pingcore_android_onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
    (void)activity;
    pingcore::AndroidNativeAppEngine::instance().on_native_window_created(window);
}

static void pingcore_android_onNativeWindowResized(ANativeActivity* activity, ANativeWindow* window) {
    (void)activity;
    pingcore::AndroidNativeAppEngine::instance().on_native_window_resized(window);
}

static void pingcore_android_onNativeWindowRedrawNeeded(ANativeActivity* activity, ANativeWindow* window) {
    (void)activity;
    pingcore::AndroidNativeAppEngine::instance().on_native_window_redraw_needed(window);
}

static void pingcore_android_onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
    (void)activity;
    (void)window;
    pingcore::AndroidNativeAppEngine::instance().on_native_window_destroyed();
}

static void pingcore_android_onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue) {
    (void)activity;
    pingcore::AndroidNativeAppEngine::instance().on_input_queue_created(queue);
}

static void pingcore_android_onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue) {
    (void)activity;
    (void)queue;
    pingcore::AndroidNativeAppEngine::instance().on_input_queue_destroyed();
}

static void pingcore_android_onContentRectChanged(ANativeActivity* activity, const ARect* rect) {
    (void)activity;
    (void)rect;
}

static void pingcore_android_onConfigurationChanged(ANativeActivity* activity) {
    (void)activity;
}

static void pingcore_android_onLowMemory(ANativeActivity* activity) {
    (void)activity;
}

// -----------------------------------------------------------------------------
// Core Required NativeActivity Entry Point: ANativeActivity_onCreate
// -----------------------------------------------------------------------------
PINGCORE_EXPORT void ANativeActivity_onCreate(
    ANativeActivity* activity,
    void* savedState,
    size_t savedStateSize
) {
    if (activity && activity->callbacks) {
        activity->callbacks->onStart = pingcore_android_onStart;
        activity->callbacks->onResume = pingcore_android_onResume;
        activity->callbacks->onSaveInstanceState = pingcore_android_onSaveInstanceState;
        activity->callbacks->onPause = pingcore_android_onPause;
        activity->callbacks->onStop = pingcore_android_onStop;
        activity->callbacks->onDestroy = pingcore_android_onDestroy;
        activity->callbacks->onWindowFocusChanged = pingcore_android_onWindowFocusChanged;
        activity->callbacks->onNativeWindowCreated = pingcore_android_onNativeWindowCreated;
        activity->callbacks->onNativeWindowResized = pingcore_android_onNativeWindowResized;
        activity->callbacks->onNativeWindowRedrawNeeded = pingcore_android_onNativeWindowRedrawNeeded;
        activity->callbacks->onNativeWindowDestroyed = pingcore_android_onNativeWindowDestroyed;
        activity->callbacks->onInputQueueCreated = pingcore_android_onInputQueueCreated;
        activity->callbacks->onInputQueueDestroyed = pingcore_android_onInputQueueDestroyed;
        activity->callbacks->onContentRectChanged = pingcore_android_onContentRectChanged;
        activity->callbacks->onConfigurationChanged = pingcore_android_onConfigurationChanged;
        activity->callbacks->onLowMemory = pingcore_android_onLowMemory;

        activity->instance = &pingcore::AndroidNativeAppEngine::instance();
    }

    pingcore::AndroidNativeAppEngine::instance().on_create(activity, savedState, savedStateSize);
}

// JNI Entry points
PINGCORE_EXPORT int32_t JNI_OnLoad(void* vm, void* reserved) {
    (void)vm;
    (void)reserved;
    return 0x00010006; /* JNI_VERSION_1_6 */
}

PINGCORE_EXPORT void JNI_OnUnload(void* vm, void* reserved) {
    (void)vm;
    (void)reserved;
    pingcore::AndroidNativeAppEngine::instance().on_destroy();
}

PINGCORE_EXPORT int32_t pingcore_android_main(void) {
    return 0;
}

} // extern "C"

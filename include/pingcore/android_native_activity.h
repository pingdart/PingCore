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
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <iomanip>

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

struct ScreenLayout {
    uint32_t width{1080};
    uint32_t height{2400};
    uint32_t bg_color{0xFF0F172A};       // Dark slate background (ABGR)
    uint32_t status_bar_color{0xFF1E293B};
    uint32_t nav_bar_color{0xFF1E293B};
    uint32_t nav_border_color{0xFF38BDF8};
    uint32_t card_bg_color{0xFF1E293B};
    uint32_t card_border_color{0xFF334155};
    uint32_t counter_box_color{0xFF0F172A};
    uint32_t counter_border_color{0xFF38BDF8};
    uint32_t button_color{0xFF2563EB};
    uint32_t button_pressed_color{0xFF1D4ED8};
    uint32_t button_reset_color{0xFF475569};
    uint32_t text_color{0xFFFFFFFF};
    uint32_t accent_color{0xFF38BDF8};
    uint32_t home_pill_color{0xFF94A3B8};

    ARect status_bar{0, 0, 1080, 80};
    ARect nav_bar{0, 80, 1080, 240};
    ARect content_card{60, 300, 1020, 1900};
    ARect counter_box{140, 480, 940, 720};
    ARect button_increment{140, 780, 940, 940};
    ARect button_reset{140, 980, 940, 1120};
    ARect footer_bar{0, 2260, 1080, 2400};
    ARect home_pill{440, 2330, 640, 2345};
};

class NativeScreenRenderer {
public:
    static void fill_rect(uint32_t* buf, uint32_t stride, uint32_t max_h, const ARect& r, uint32_t color) {
        int32_t left = std::max<int32_t>(0, r.left);
        int32_t right = std::min<int32_t>(static_cast<int32_t>(stride), r.right);
        int32_t top = std::max<int32_t>(0, r.top);
        int32_t bottom = std::min<int32_t>(static_cast<int32_t>(max_h), r.bottom);
        for (int32_t y = top; y < bottom; ++y) {
            uint32_t* row = buf + static_cast<size_t>(y) * stride;
            for (int32_t x = left; x < right; ++x) {
                row[x] = color;
            }
        }
    }

    static void draw_outline(uint32_t* buf, uint32_t stride, uint32_t max_h, const ARect& r, uint32_t color, int thickness = 4) {
        ARect top_border{r.left, r.top, r.right, r.top + thickness};
        ARect bottom_border{r.left, r.bottom - thickness, r.right, r.bottom};
        ARect left_border{r.left, r.top, r.left + thickness, r.bottom};
        ARect right_border{r.right - thickness, r.top, r.right, r.bottom};
        fill_rect(buf, stride, max_h, top_border, color);
        fill_rect(buf, stride, max_h, bottom_border, color);
        fill_rect(buf, stride, max_h, left_border, color);
        fill_rect(buf, stride, max_h, right_border, color);
    }

    static void draw_digit(uint32_t* buf, uint32_t stride, uint32_t max_h, int digit, int cx, int cy, int scale, uint32_t color) {
        static const uint8_t font[10][7] = {
            {0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F}, // 0
            {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
            {0x1F, 0x01, 0x01, 0x1F, 0x10, 0x10, 0x1F}, // 2
            {0x1F, 0x01, 0x01, 0x0F, 0x01, 0x01, 0x1F}, // 3
            {0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01}, // 4
            {0x1F, 0x10, 0x10, 0x1F, 0x01, 0x01, 0x1F}, // 5
            {0x1F, 0x10, 0x10, 0x1F, 0x11, 0x11, 0x1F}, // 6
            {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
            {0x1F, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x1F}, // 8
            {0x1F, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x1F}  // 9
        };
        if (digit < 0 || digit > 9) return;
        for (int r = 0; r < 7; ++r) {
            uint8_t row = font[digit][r];
            for (int c = 0; c < 5; ++c) {
                if (row & (0x10 >> c)) {
                    ARect pixel_block{cx + c * scale, cy + r * scale, cx + (c + 1) * scale, cy + (r + 1) * scale};
                    fill_rect(buf, stride, max_h, pixel_block, color);
                }
            }
        }
    }

    static void render_full_screen(uint32_t* buf, uint32_t w, uint32_t h, uint32_t stride, int counter_val, bool pressed = false) {
        ScreenLayout layout;
        layout.width = w;
        layout.height = h;
        layout.status_bar.right = static_cast<int32_t>(w);
        layout.nav_bar.right = static_cast<int32_t>(w);
        layout.footer_bar.right = static_cast<int32_t>(w);
        layout.footer_bar.top = static_cast<int32_t>(h > 200 ? h - 140 : h);
        layout.footer_bar.bottom = static_cast<int32_t>(h);
        layout.home_pill.left = static_cast<int32_t>(w / 2 - 100);
        layout.home_pill.right = static_cast<int32_t>(w / 2 + 100);
        layout.home_pill.top = static_cast<int32_t>(h > 100 ? h - 70 : h);
        layout.home_pill.bottom = static_cast<int32_t>(h > 80 ? h - 55 : h);
        layout.content_card.right = static_cast<int32_t>(w > 120 ? w - 60 : w);
        layout.content_card.bottom = static_cast<int32_t>(h > 300 ? h - 200 : h);

        // Background fill
        std::fill(buf, buf + (static_cast<size_t>(stride) * h), layout.bg_color);

        // Status bar
        fill_rect(buf, stride, h, layout.status_bar, layout.status_bar_color);

        // Navigation header
        fill_rect(buf, stride, h, layout.nav_bar, layout.nav_bar_color);
        ARect nav_border{0, layout.nav_bar.bottom - 4, static_cast<int32_t>(w), layout.nav_bar.bottom};
        fill_rect(buf, stride, h, nav_border, layout.nav_border_color);

        // Main content card
        fill_rect(buf, stride, h, layout.content_card, layout.card_bg_color);
        draw_outline(buf, stride, h, layout.content_card, layout.card_border_color, 2);

        // Counter display container
        fill_rect(buf, stride, h, layout.counter_box, layout.counter_box_color);
        draw_outline(buf, stride, h, layout.counter_box, layout.counter_border_color, 4);

        // Render digits of counter
        int d = std::abs(counter_val) % 100;
        int d1 = d / 10;
        int d2 = d % 10;
        draw_digit(buf, stride, h, d1, 460, 530, 14, layout.accent_color);
        draw_digit(buf, stride, h, d2, 560, 530, 14, layout.accent_color);

        // Interactive Increment Button
        uint32_t btn_col = pressed ? layout.button_pressed_color : layout.button_color;
        fill_rect(buf, stride, h, layout.button_increment, btn_col);
        draw_outline(buf, stride, h, layout.button_increment, layout.accent_color, 2);

        // Reset Button
        fill_rect(buf, stride, h, layout.button_reset, layout.button_reset_color);

        // Footer / bottom navigation bar
        fill_rect(buf, stride, h, layout.footer_bar, layout.status_bar_color);
        fill_rect(buf, stride, h, layout.home_pill, layout.home_pill_color);
    }

    // Pure C++ uncompressed PNG exporter
    static bool write_png(const std::string& path, const uint32_t* buf, uint32_t w, uint32_t h, uint32_t stride) {
        std::ofstream out(path, std::ios::binary);
        if (!out) return false;

        auto crc32_calc = [](const uint8_t* data, size_t len) -> uint32_t {
            uint32_t crc = 0xFFFFFFFF;
            for (size_t i = 0; i < len; ++i) {
                crc ^= data[i];
                for (int j = 0; j < 8; ++j) {
                    crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
                }
            }
            return ~crc;
        };

        auto adler32_calc = [](const uint8_t* data, size_t len) -> uint32_t {
            uint32_t s1 = 1, s2 = 0;
            for (size_t i = 0; i < len; ++i) {
                s1 = (s1 + data[i]) % 65521;
                s2 = (s2 + s1) % 65521;
            }
            return (s2 << 16) | s1;
        };

        auto write_chunk = [&](const char* type, const uint8_t* data, uint32_t len) {
            uint32_t be_len = __builtin_bswap32(len);
            out.write(reinterpret_cast<const char*>(&be_len), 4);
            out.write(type, 4);
            if (len > 0) out.write(reinterpret_cast<const char*>(data), len);
            std::vector<uint8_t> crc_buf(4 + len);
            std::memcpy(crc_buf.data(), type, 4);
            if (len > 0) std::memcpy(crc_buf.data() + 4, data, len);
            uint32_t crc = crc32_calc(crc_buf.data(), crc_buf.size());
            uint32_t be_crc = __builtin_bswap32(crc);
            out.write(reinterpret_cast<const char*>(&be_crc), 4);
        };

        // PNG Header
        const uint8_t png_sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
        out.write(reinterpret_cast<const char*>(png_sig), 8);

        // IHDR
        uint8_t ihdr[13];
        uint32_t be_w = __builtin_bswap32(w);
        uint32_t be_h = __builtin_bswap32(h);
        std::memcpy(ihdr, &be_w, 4);
        std::memcpy(ihdr + 4, &be_h, 4);
        ihdr[8] = 8;  // bit depth
        ihdr[9] = 6;  // RGBA
        ihdr[10] = 0; // compression
        ihdr[11] = 0; // filter
        ihdr[12] = 0; // interlace
        write_chunk("IHDR", ihdr, 13);

        // Raw scanlines
        std::vector<uint8_t> raw;
        raw.reserve(h * (1 + w * 4));
        for (uint32_t y = 0; y < h; ++y) {
            raw.push_back(0); // filter: None
            const uint32_t* row = buf + y * stride;
            for (uint32_t x = 0; x < w; ++x) {
                uint32_t c = row[x];
                uint8_t r = c & 0xFF;
                uint8_t g = (c >> 8) & 0xFF;
                uint8_t b = (c >> 16) & 0xFF;
                uint8_t a = (c >> 24) & 0xFF;
                raw.push_back(r);
                raw.push_back(g);
                raw.push_back(b);
                raw.push_back(a > 0 ? a : 0xFF);
            }
        }

        // DEFLATE stream with uncompressed blocks (RFC 1950 + RFC 1951)
        std::vector<uint8_t> zlib;
        zlib.push_back(0x78);
        zlib.push_back(0x01);
        size_t pos = 0;
        while (pos < raw.size()) {
            uint16_t chunk_len = static_cast<uint16_t>(std::min<size_t>(65535, raw.size() - pos));
            bool is_final = (pos + chunk_len == raw.size());
            zlib.push_back(is_final ? 1 : 0);
            uint16_t nlen = static_cast<uint16_t>(~chunk_len);
            zlib.push_back(chunk_len & 0xFF);
            zlib.push_back((chunk_len >> 8) & 0xFF);
            zlib.push_back(nlen & 0xFF);
            zlib.push_back((nlen >> 8) & 0xFF);
            zlib.insert(zlib.end(), raw.begin() + static_cast<std::ptrdiff_t>(pos), raw.begin() + static_cast<std::ptrdiff_t>(pos + chunk_len));
            pos += chunk_len;
        }
        uint32_t adler = adler32_calc(raw.data(), raw.size());
        uint32_t be_adler = __builtin_bswap32(adler);
        const uint8_t* adler_bytes = reinterpret_cast<const uint8_t*>(&be_adler);
        zlib.insert(zlib.end(), adler_bytes, adler_bytes + 4);

        write_chunk("IDAT", zlib.data(), static_cast<uint32_t>(zlib.size()));
        write_chunk("IEND", nullptr, 0);
        return true;
    }
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

    int reactive_counter() const noexcept { return reactive_counter_; }
    void set_reactive_counter(int val) noexcept { reactive_counter_ = val; }

    bool handle_touch_event(int32_t x, int32_t y, int32_t action) {
        if (action == 0) { // ACTION_DOWN
            if (x >= 140 && x <= 940 && y >= 780 && y <= 940) {
                reactive_counter_++;
                button_pressed_ = true;
                render_frame();
                return true;
            }
            if (x >= 140 && x <= 940 && y >= 980 && y <= 1120) {
                reactive_counter_ = 0;
                button_pressed_ = true;
                render_frame();
                return true;
            }
        } else if (action == 1) { // ACTION_UP
            if (button_pressed_) {
                button_pressed_ = false;
                render_frame();
                return true;
            }
        }
        return false;
    }

    void render_screen_to_buffer(uint32_t* buf, uint32_t w, uint32_t h, uint32_t stride) {
        NativeScreenRenderer::render_full_screen(buf, w, h, stride, reactive_counter_, button_pressed_);
        render_frame();
    }

    bool capture_screenshot_png(const std::string& path, const uint32_t* buf, uint32_t w, uint32_t h, uint32_t stride) {
        return NativeScreenRenderer::write_png(path, buf, w, h, stride);
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
        reactive_counter_ = 0;
        button_pressed_ = false;
    }

private:
    AndroidNativeAppEngine() = default;
    AndroidNativeAppState state_;
    int reactive_counter_{0};
    bool button_pressed_{false};
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

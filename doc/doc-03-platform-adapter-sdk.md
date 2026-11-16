# PingDart Framework
## DOC-03: Platform Adapter SDK Specification (Windows & Linux)

**Status:** Technical Architecture Specification  
**Version:** 1.0  
**Language Baseline:** C++20  
**Parent Documents:** [PingDart PRD](prd.md) · [DOC-01: Core Runtime Architecture](doc-01-core-runtime-architecture.md)  
**Target Phase:** Phase 1 (Desktop Proof-of-Concept)

---

# 1. Executive Summary

The **Platform Adapter SDK** defines the **Platform Abstraction Layer (PAL)** that connects the platform-independent PingDart runtime (DOC-01), rendering engine (DOC-02), and widget tree (DOC-05) to operating-system native host services.

For **Phase 1**, this specification focuses on **Windows (Win32)** and **Linux (Wayland with automatic X11 fallback)**.

```text
+--------------------------------------------------------------------------+
|                     PingDart Application / Widget Tree                  |
+--------------------------------------------------------------------------+
                                     |
+--------------------------------------------------------------------------+
|                      PingDart Core Runtime (DOC-01)                      |
|                Event Loop · Task Scheduler · RefPtr · Result             |
+--------------------------------------------------------------------------+
                                     |
+--------------------------------------------------------------------------+
|                  DOC-03: Platform Abstraction Layer (PAL)                |
|                                                                          |
|  [IPlatformHost]      [IWindow]              [ISurfaceProvider]          |
|  [IInputDispatcher]   [IPlatformEventPump]   [IDisplayManager]           |
|  [ICapabilityProvider]                                                   |
+--------------------------------------------------------------------------+
                  |                                      |
                  v                                      v
+-----------------------------------+  +-----------------------------------+
|          Windows Adapter          |  |           Linux Adapter           |
|  - Win32 (HWND, WindowProc)       |  |  - Wayland (wl_surface, xdg_shell)|
|  - Raw Input & WM_MOUSE/KEYBOARD  |  |  - X11 (Xlib/XCB, Window, EWMH)   |
|  - MsgWaitForMultipleObjectsEx    |  |  - epoll fd integration           |
|  - Per-Monitor v2 DPI Awareness   |  |  - xkbcommon input normalization  |
+-----------------------------------+  +-----------------------------------+
```

---

# 2. Core Architectural Design Decisions

### 2.1 Design Decision 1: Linux Backend Selection (Wayland vs. X11)
To support modern Linux desktop environments (GNOME, KDE Plasma, Sway) while retaining compatibility with legacy and enterprise installations:
1. **Dynamic Startup Detection:**
   - The Linux adapter checks environment variable `PINGDART_BACKEND` (`wayland` or `x11`).
   - If unset, the adapter checks for `WAYLAND_DISPLAY` and attempts connection via `wl_display_connect(nullptr)`.
   - If Wayland connection fails or is absent, the adapter inspects `DISPLAY` and connects via `XOpenDisplay(nullptr)`.
   - If both fail, initialization returns `StatusCode::Unavailable` with actionable diagnostic details.
2. **Library Loading Strategy:**
   - Both Wayland (`libwayland-client.so.0`) and X11 (`libX11.so.6`, `libX11-xcb.so.1`) are loaded dynamically (`dlopen`/`dlsym`) or linked with weak symbols, ensuring a single binary runs on systems lacking Wayland or X11 libraries.

### 2.2 Design Decision 2: Event Loop Ownership & Non-Competing Pump
A major failure mode in cross-platform frameworks is competing event loops between the runtime scheduler and OS message pumps.
- **Single Master Event Loop:** The DOC-01 `EventLoop` remains the sole master event loop. Native platform event processing does *not* run a secondary blocking loop.
- **Windows Integration:** The Win32 message pump integrates into DOC-01 via `MsgWaitForMultipleObjectsEx`:
  - It explicitly waits on the DOC-01 runtime wakeup handle (an auto-reset Win32 Event handle signaled by `wake_up()`).
  - It strictly respects the next scheduled timer deadline calculated by DOC-01's timer queue (passing the exact remaining milliseconds as the wait timeout).
  - When messages or wakeups arrive, the platform event pump dispatches non-blocking `PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)` until the Win32 queue is drained before yielding control back to the scheduler.
- **Linux Wayland Integration:**
  - Watching the Wayland socket with raw `epoll` alone is **insufficient and prone to multi-thread deadlocks and lost events**.
  - PingDart strictly implements the canonical Wayland multi-thread event protocol with retry semantics:
    1. Enter preparation loop:
       ```cpp
       while (wl_display_prepare_read(display) != 0) {
           // Queue had pending events that prevented preparation; dispatch and retry
           wl_display_dispatch_pending(display);
       }
       ```
    2. Call `wl_display_flush(display)` to push client requests to the compositor socket.
    3. Execute `epoll_wait()` bounded by the next DOC-01 timer deadline and wakeup eventfd.
    4. Post-wait state resolution:
       - If the Wayland socket was signaled as readable: call `wl_display_read_events(display)`.
       - If `epoll_wait()` woke due to another event (e.g., worker `eventfd`, timer expiry, or `EINTR` signal interruption): call `wl_display_cancel_read(display)` to balance the read intent without blocking.
    5. Finally, execute `wl_display_dispatch_pending(display)` on the UI thread to deliver dispatched events to `IInputDispatcher`.
- **Linux X11 Integration:** The X connection file descriptor (`XConnectionNumber(dpy)`) is monitored via `epoll`. Incoming events are drained using `XPending(dpy)` and `XNextEvent(dpy)` before scheduler task execution.

### 2.3 Design Decision 3: GPU Device Independence & Surface Lifetime Contract
DOC-03 **does not own, create, or initialize GPU graphics contexts** (Vulkan `VkDevice` or Direct3D 12 `ID3D12Device`). 
- DOC-03 is strictly responsible for providing the **native window surface token** (`ISurfaceProvider`) and forwarding window lifecycle events (resizing, occlusion, dpi changes).
- **Surface Lifetime & Generation Tracking:**
  - Native surfaces are strictly owned by `IWindow` and have distinct lifecycle phases.
  - Every surface holds a monotonically increasing `generation_id` (`uint64_t`). Whenever a window is recreated, redocked, or changes presentation format, the `generation_id` increments.
  - The rendering engine (DOC-02) checks the `generation_id` prior to frame submission. If the ID changed, DOC-02 recreates its swapchain.
  - **Thread-Affinity Rule:** Window and native surface creation/destruction are strictly confined to the **UI Thread**. When the UI thread initiates window destruction, an explicit fence/synchronization signal ensures the Render Thread finishes any in-flight GPU presentation before the underlying native surface handle (`HWND`, `wl_surface`, `Window`) is destroyed.

---

# 3. PAL Interface Contracts

All PAL interfaces reside in the `pingdart::pal` namespace, use C++20 standard constructs, and return explicit `pingdart::core::Result<T>` values.

### 3.1 Native Types & Surface Token (`pingdart/pal/native_types.h`)

```cpp
#pragma once
#include <cstdint>
#include <variant>
#include <string>

namespace pingdart::pal {

enum class PlatformType : uint8_t {
    Windows,
    LinuxWayland,
    LinuxX11
};

struct Win32SurfaceDescriptor {
    void* hwnd;        // HWND
    void* hinstance;   // HINSTANCE
};

struct WaylandSurfaceDescriptor {
    void* display;     // struct wl_display*
    void* surface;     // struct wl_surface*
};

struct X11SurfaceDescriptor {
    void* display;     // Display*
    uint64_t window;   // Window (XID)
    void* visual;      // Visual*
};

// Type-safe tagged union of platform surface handles
using NativeSurfaceToken = std::variant<
    Win32SurfaceDescriptor,
    WaylandSurfaceDescriptor,
    X11SurfaceDescriptor
>;

struct NativeSurfaceHandle {
    NativeSurfaceToken token;
    uint64_t generation_id{1}; // Increments on recreate/resize/redock
    bool is_valid{true};
};

struct WindowSize {
    uint32_t width{0};
    uint32_t height{0};
};

struct WindowPoint {
    int32_t x{0};
    int32_t y{0};
};

struct DisplayMode {
    uint32_t width{0};
    uint32_t height{0};
    uint32_t refresh_rate_millihz{60000}; // e.g. 60000 = 60.00 Hz, 144000 = 144.00 Hz
    float scale_factor{1.0f};             // 1.0 = 100%, 1.5 = 150%, 2.0 = 200%
};

} // namespace pingdart::pal
```

### 3.2 Normalized Input Events (`pingdart/pal/input_events.h`)

```cpp
#pragma once
#include <cstdint>
#include <chrono>

namespace pingdart::pal {

enum class EventType : uint16_t {
    PointerDown,
    PointerUp,
    PointerMove,
    PointerEnter,
    PointerLeave,
    PointerWheel,
    KeyDown,
    KeyUp,
    CharacterInput,
    WindowFocusGained,
    WindowFocusLost,
    WindowResized,
    WindowMoved,
    WindowCloseRequested,
    WindowDpiChanged
};

enum class PointerButton : uint8_t {
    None = 0,
    Left = 1,
    Right = 2,
    Middle = 3,
    Back = 4,
    Forward = 5
};

enum class ModifierKey : uint8_t {
    None     = 0,
    Shift    = 1 << 0,
    Control  = 1 << 1,
    Alt      = 1 << 2,
    Meta     = 1 << 3, // Windows Key / Command / Super
    CapsLock = 1 << 4
};

inline ModifierKey operator|(ModifierKey a, ModifierKey b) {
    return static_cast<ModifierKey>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool operator&(ModifierKey a, ModifierKey b) {
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

struct PointerEvent {
    EventType type;
    uint32_t pointer_id{0};       // 0 for default mouse, 1+ for multi-touch/stylus
    double x{0.0};                // Physical or logical window X coordinate
    double y{0.0};                // Physical or logical window Y coordinate
    double delta_x{0.0};          // Mouse wheel horizontal scroll delta
    double delta_y{0.0};          // Mouse wheel vertical scroll delta
    PointerButton button{PointerButton::None};
    uint8_t buttons_down{0};      // Bitmask of active PointerButton values
    ModifierKey modifiers{ModifierKey::None};
    float pressure{1.0f};         // 0.0f to 1.0f (stylus/touch pressure)
    uint64_t timestamp_us{0};     // Microseconds since steady_clock epoch
};

struct KeyboardEvent {
    EventType type;
    uint32_t key_code{0};         // Physical scan code / hardware scancode
    uint32_t virtual_key{0};      // Normalized virtual key (Key::A, Key::Escape)
    uint32_t unicode_codepoint{0};// Unicode character for CharacterInput events
    ModifierKey modifiers{ModifierKey::None};
    bool is_repeat{false};
    uint64_t timestamp_us{0};
};

struct WindowLifecycleEvent {
    EventType type;
    uint32_t width{0};
    uint32_t height{0};
    float scale_factor{1.0f};
    uint64_t timestamp_us{0};
};

} // namespace pingdart::pal
```

### 3.3 Core PAL Interfaces (`pingdart/pal/*.h`)

#### A. `IPlatformHost` (`pingdart/pal/i_platform_host.h`)
```cpp
#pragma once
#include "native_types.h"
#include <pingdart/core/result.h>
#include <memory>

namespace pingdart::pal {

class IWindow;
class IDisplayManager;
class ICapabilityProvider;
class IPlatformEventPump;

class IPlatformHost {
public:
    virtual ~IPlatformHost() = default;

    // Initializes platform subsystem, connects to display server (Win32 / Wayland / X11)
    [[nodiscard]] virtual core::Status initialize() = 0;

    // Releases all platform resources, closes display connections
    virtual void shutdown() = 0;

    // Returns platform type
    [[nodiscard]] virtual PlatformType platform_type() const noexcept = 0;

    // Subsystem accessors
    [[nodiscard]] virtual IDisplayManager& display_manager() = 0;
    [[nodiscard]] virtual ICapabilityProvider& capability_provider() = 0;
    [[nodiscard]] virtual IPlatformEventPump& event_pump() = 0;

    // Factory for creating a top-level native window
    [[nodiscard]] virtual core::Result<std::unique_ptr<IWindow>> create_window(
        const std::string& title,
        const WindowSize& initial_size) = 0;

    // Factory to instantiate platform host
    static std::unique_ptr<IPlatformHost> create();
};

} // namespace pingdart::pal
```

#### B. `IWindow` and `ISurfaceProvider` (`pingdart/pal/i_window.h`)
```cpp
#pragma once
#include "native_types.h"
#include <pingdart/core/result.h>
#include <string>
#include <string_view>
#include <functional>

namespace pingdart::pal {

class IInputDispatcher;

class ISurfaceProvider {
public:
    virtual ~ISurfaceProvider() = default;

    // Exposes native handle with generation ID required by DOC-02 for swapchain lifecycle
    [[nodiscard]] virtual NativeSurfaceHandle native_surface_handle() const noexcept = 0;

    // Monotonically increasing generation counter
    [[nodiscard]] virtual uint64_t surface_generation_id() const noexcept = 0;

    // Returns physical drawable dimensions in pixels
    [[nodiscard]] virtual WindowSize physical_size() const noexcept = 0;

    // Returns display scaling factor (e.g. 1.5 for 150% DPI)
    [[nodiscard]] virtual float scale_factor() const noexcept = 0;
};

class IWindow : public ISurfaceProvider {
public:
    ~IWindow() override = default;

    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void close() = 0;

    virtual void set_title(std::string_view title) = 0;
    virtual void set_size(const WindowSize& size) = 0;
    virtual void set_position(const WindowPoint& position) = 0;

    [[nodiscard]] virtual bool is_visible() const noexcept = 0;
    [[nodiscard]] virtual bool is_minimized() const noexcept = 0;
    [[nodiscard]] virtual bool is_maximized() const noexcept = 0;

    // Registers listener for incoming normalized input events
    virtual void set_input_dispatcher(IInputDispatcher* dispatcher) = 0;
};

} // namespace pingdart::pal
```

#### C. `IInputDispatcher` (`pingdart/pal/i_input_dispatcher.h`)
```cpp
#pragma once
#include "input_events.h"

namespace pingdart::pal {

class IInputDispatcher {
public:
    virtual ~IInputDispatcher() = default;

    virtual void dispatch_pointer_event(const PointerEvent& event) = 0;
    virtual void dispatch_keyboard_event(const KeyboardEvent& event) = 0;
    virtual void dispatch_lifecycle_event(const WindowLifecycleEvent& event) = 0;
};

} // namespace pingdart::pal
```

#### D. `IPlatformEventPump` (`pingdart/pal/i_platform_event_pump.h`)
```cpp
#pragma once
#include <chrono>

namespace pingdart::pal {

class IPlatformEventPump {
public:
    virtual ~IPlatformEventPump() = default;

    // Non-blocking pump: drains OS queue and returns immediately
    virtual void poll_events() = 0;

    // Blocks until an OS event arrives or the specified timeout expires
    virtual bool wait_for_events(std::chrono::milliseconds timeout) = 0;

    // Wakes up wait_for_events from another thread
    virtual void wake_up() = 0;
};

} // namespace pingdart::pal
```

#### E. `IDisplayManager` (`pingdart/pal/i_display_manager.h`)
```cpp
#pragma once
#include "native_types.h"
#include <vector>
#include <string>

namespace pingdart::pal {

struct DisplayInfo {
    uint32_t id{0};
    std::string name;
    DisplayMode current_mode;
    WindowPoint top_left_position;
    bool is_primary{false};
};

class IDisplayManager {
public:
    virtual ~IDisplayManager() = default;

    [[nodiscard]] virtual std::vector<DisplayInfo> enumerate_displays() = 0;
    [[nodiscard]] virtual DisplayInfo primary_display() = 0;
};

} // namespace pingdart::pal
```

#### F. `ICapabilityProvider` (`pingdart/pal/i_capability_provider.h`)
```cpp
#pragma once
#include <cstdint>

namespace pingdart::pal {

enum class CapabilityType : uint32_t {
    TouchInput,
    StylusPressure,
    HighDpiDisplay,
    MultiWindow,
    SystemTray,
    Clipboard,
    FilePicker,
    VulkanSurface,
    Direct3D12Surface,
    WaylandDecoration
};

enum class CapabilityState : uint8_t {
    Unavailable,        // Tier 1: Hardware physically missing
    NotSupportedByOS,   // Tier 2: Host OS lacks API
    PermissionDenied,   // Tier 3: User/policy blocked
    Ready               // Tier 4: Immediately ready to use
};

class ICapabilityProvider {
public:
    virtual ~ICapabilityProvider() = default;

    [[nodiscard]] virtual CapabilityState query_capability(CapabilityType type) const noexcept = 0;
};

} // namespace pingdart::pal
```

---

# 4. Windows Adapter Implementation Architecture

### 4.1 Window Class & Procedure
1. **DPI Awareness:** Process is initialized with `SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)`.
2. **Window Registration:** Registers `WNDCLASSEXW` with unique `lpszClassName = L"PingDart_Window_Class"`.
3. **Window Styles:** `WS_OVERLAPPEDWINDOW | WS_VISIBLE`. Window style handles client-size versus window-rect adjustments via `AdjustWindowRectExForDpi`.
4. **Message Handling (`WindowProc`):**
   - `WM_DPICHANGED`: Extracts new scale factor and suggested `RECT` from `lParam`, updates internal scale, and issues `SetWindowPos`.
   - `WM_SIZE`: Extracts client dimensions from `LOWORD(lParam)` / `HIWORD(lParam)`, dispatches `EventType::WindowResized`.
   - `WM_CLOSE`: Intercepts user close, dispatches `EventType::WindowCloseRequested` to PingDart widget tree (allowing "save unsaved changes" confirmation) before calling `DestroyWindow`.
   - `WM_MOUSEMOVE`, `WM_LBUTTONDOWN/UP`, `WM_RBUTTONDOWN/UP`: Normalized into `PointerEvent`. Utilizes `TrackMouseEvent` (`TME_LEAVE`) for reliable cursor exit detection.
   - `WM_KEYDOWN`, `WM_KEYUP`, `WM_CHAR`: Normalized into `KeyboardEvent` with virtual keys and Unicode characters.
   - `WM_ERASEBKGND`: Intercepted and returns `1` (prevents screen flicker during resize before GPU draw).

### 4.2 Win32 Event Pump Integration
- `IPlatformEventPump::poll_events()` executes non-blocking message draining:
  ```cpp
  MSG msg;
  while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
  }
  ```
- `IPlatformEventPump::wait_for_events(timeout)` binds directly to the DOC-01 scheduler:
  ```cpp
  HANDLE handles[1] = { g_wakeup_event_handle };
  DWORD wait_ms = timeout.count() >= 0 ? static_cast<DWORD>(timeout.count()) : INFINITE;
  DWORD result = MsgWaitForMultipleObjectsEx(1, handles, wait_ms, QS_ALLINPUT | QS_POSTMESSAGE, MWMO_ALERTABLE);
  if (result == WAIT_OBJECT_0) {
      // Runtime wakeup signaled from background worker
      ResetEvent(g_wakeup_event_handle);
  }
  // Immediately drain Win32 queue
  poll_events();
  ```
- `wake_up()` signals `SetEvent(g_wakeup_event_handle)`.

---

# 5. Linux Adapter Implementation Architecture

### 5.1 Wayland Backend
1. **Connection & Registry:**
   - Connects to compositor via `wl_display_connect(nullptr)`.
   - Binds globals from `wl_registry`: `wl_compositor`, `wl_shm`, `xdg_wm_base`, `wl_seat`, `wp_fractional_scale_manager_v1`, `wp_viewporter`, and `zwp_text_input_manager_v3`.
2. **Surface Creation & Fractional Scaling:**
   - Creates `wl_surface` from compositor.
   - Binds `wp_fractional_scale_v1` to listen for compositor-driven 120ths fractional DPI scaling (e.g. 150 scale = 1.25x).
   - Allocates `xdg_surface` and `xdg_toplevel` via `xdg_wm_base`.
   - Handles `xdg_surface.configure` by acknowledging with `xdg_surface_ack_configure` and committing.
3. **Seat Capabilities & Input Handling (`wl_seat`):**
   - Implements `wl_seat_listener::capabilities`: dynamically adds or tears down `wl_pointer`, `wl_keyboard`, and `wl_touch` when peripherals are plugged in or removed.
   - **XKB Keyboard Translation:** Uses `libxkbcommon` (`xkb_context_new`, `xkb_keymap_new_from_string`, `xkb_state_new`) to translate hardware evdev scancodes, update modifier masks (Shift, Ctrl, Alt, Mod4/Super, CapsLock), and map dead keys.
   - **Text Composition / IME:** Implements `zwp_text_input_v3` protocol, forwarding pre-edit strings, cursor offsets, and committed Unicode text directly to PingDart input dispatcher.
   - **Pointer Scaling:** Scales `wl_pointer` coordinates by the active fractional scale factor before populating `PointerEvent`.
4. **Window Decorations:**
   - Negotiates client-side decorations (CSD) or uses `zxdg_decoration_manager_v1` for server-side decorations (SSD) when supported by compositor (KDE/Sway).

### 5.2 X11 Fallback Backend
1. **Connection & Visual:**
   - Connects to X server via `XOpenDisplay(nullptr)`.
   - Queries standard TrueColor visual and creates colormap.
2. **Window Creation & Protocols:**
   - Calls `XCreateWindow` with standard event masks (`ExposureMask`, `StructureNotifyMask`, `KeyPressMask`, `KeyReleaseMask`, `PointerMotionMask`, `ButtonPressMask`, `ButtonReleaseMask`).
   - Sets `WM_PROTOCOLS` with `WM_DELETE_WINDOW` atom for clean close interception.
3. **Event Pump Integration:**
   - Native file descriptor `XConnectionNumber(dpy)` is registered in `epoll`.

---

# 6. CMake Target Structure

```cmake
# cmake/Modules/PingDartPal.cmake

add_library(pingdart_pal STATIC)

target_include_directories(pingdart_pal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pingdart_pal PUBLIC
    pingdart_core
)

if(WIN32)
    target_sources(pingdart_pal PRIVATE
        src/pal/windows/win32_platform_host.cpp
        src/pal/windows/win32_window.cpp
        src/pal/windows/win32_event_pump.cpp
        src/pal/windows/win32_display_manager.cpp
        src/pal/windows/win32_capability_provider.cpp
    )
    target_link_libraries(pingdart_pal PRIVATE
        user32
        gdi32
        dwmapi
        shcore
    )
elseif(UNIX AND NOT APPLE)
    target_sources(pingdart_pal PRIVATE
        src/pal/linux/linux_platform_host.cpp
        src/pal/linux/wayland/wayland_window.cpp
        src/pal/linux/wayland/wayland_event_pump.cpp
        src/pal/linux/x11/x11_window.cpp
        src/pal/linux/x11/x11_event_pump.cpp
        src/pal/linux/linux_display_manager.cpp
        src/pal/linux/linux_capability_provider.cpp
    )
    target_link_libraries(pingdart_pal PRIVATE
        wayland-client
        xkbcommon
        X11
        X11-xcb
        dl
        pthread
    )
endif()
```

---

# 7. Desktop Proof-of-Concept Acceptance Test Specification

The initial implementation of DOC-03 is accepted when the following test suite passes:

### Test Case 1: Host Initialization & Display Enumeration
* **Procedure:** Initialize `IPlatformHost`. Query `IDisplayManager`.
* **Pass Condition:** Returns valid display dimensions, primary monitor flag, and active scaling factor (DPI >= 1.0).

### Test Case 2: Window Lifecycle & Sizing
* **Procedure:** Create window (800x600, title "PingDart POC"). Call `show()`. Verify visibility. Issue `set_size(1024, 768)`.
* **Pass Condition:** Window renders on screen with exact client dimensions. Resizing triggers `WindowLifecycleEvent::WindowResized` without deadlock.

### Test Case 3: Surface Token Verification for DOC-02
* **Procedure:** Call `ISurfaceProvider::native_surface_token()`.
* **Pass Condition:** 
  - On Windows: Token holds `Win32SurfaceDescriptor` with valid, non-null `hwnd`.
  - On Linux Wayland: Token holds `WaylandSurfaceDescriptor` with valid `wl_display` and `wl_surface`.
  - On Linux X11: Token holds `X11SurfaceDescriptor` with valid `Display*` and non-zero `Window`.

### Test Case 4: Input Normalization Loop
* **Procedure:** Generate physical mouse movements, clicks, and keyboard strokes.
* **Pass Condition:** Normalized `PointerEvent` and `KeyboardEvent` delivered to registered `IInputDispatcher` matching physical coordinates and key codes.

### Test Case 5: Clean Tear-down & Resource Release
* **Procedure:** Issue `IWindow::close()`. Call `IPlatformHost::shutdown()`.
* **Pass Condition:** Window closes instantly, process exits with status 0, and memory/handle leak sanitizers (Valgrind / ASan) report zero leaks.

---

# 8. Next Technical Step

With **DOC-03** defined, the next architecture specification is **DOC-02: Rendering Engine Architecture**, which specifies:
1. Vulkan swapchain initialization from `NativeSurfaceToken`.
2. Direct3D 12 swapchain initialization on Windows.
3. Skia integration / display list rasterization pipeline.
4. Frame synchronization with the DOC-01 scheduler.

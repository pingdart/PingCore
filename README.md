# PingDart Framework
## Complete Product Requirements Document (PRD) & Technical Architecture

**Product:** PingDart Framework  
**Category:** Cross-Platform Application Development Framework  
**Version:** 1.0  
**Status:** Proposed – Architecture and Development Specification  
**Core Technology:** C++  
**Compiler Infrastructure:** LLVM / Clang, with integration evaluation of PingDartCore (Rust)  
**Target Platforms:** Android, iOS, Windows, macOS, Linux, Web (Phase 1-5); Android TV, Google TV, watchOS, Wear OS, tvOS, visionOS, Embedded Linux, QNX (Phase 6-9)  
**Architecture:** Shared Core + Tri-Modal Rendering + Native Platform Adapters  
**Development Philosophy:** Write Once, Build Everywhere

---

# 1. Executive Summary

PingDart Framework is a next-generation cross-platform application development framework designed to allow developers to create applications for multiple operating systems using a shared codebase.

The framework will provide:

- A high-performance C++ application engine.
- A custom UI rendering engine.
- A reusable widget and component system.
- A cross-platform layout engine.
- A compiler and build system.
- Native platform integration.
- A package and plugin ecosystem.
- Hot reload and debugging tools.
- WebAssembly support.
- An integrated developer CLI.

The goal is to create an independent technology ecosystem comparable in scope to Flutter, Qt and React Native, with its own runtime, rendering system, APIs, tooling and developer experience.

## 1.1 Product Vision

Developers should be able to create an application once and deploy it across:

1. Android
2. iOS
3. Windows
4. macOS
5. Linux
6. Web browsers

The framework should minimize platform-specific code while retaining access to native operating-system capabilities.

## 1.2 Core Principles

- Performance-first architecture.
- Memory safety and predictable resource management.
- Cross-platform consistency.
- Native API accessibility.
- Modular architecture.
- Extensible plugin ecosystem.
- Open and documented SDK.
- Backward compatibility.
- Developer productivity.
- Production-grade security.

---

# 2. Product Goals

## 2.1 Primary Goals

1. Build a C++ cross-platform application framework.
2. Create an independent UI rendering engine.
3. Create a reusable widget library.
4. Develop a compiler and build pipeline.
5. Support six major deployment targets.
6. Provide a consistent developer API.
7. Support native device capabilities.
8. Enable third-party packages and plugins.
9. Build an IDE and debugging ecosystem.
10. Support commercial and enterprise applications.

## 2.2 Non-Goals

The initial versions will not attempt to:

- Replace Android or iOS operating systems.
- Replace LLVM or Clang unnecessarily.
- Implement every operating-system API.
- Guarantee identical rendering performance on all hardware.
- Provide a complete replacement for native platform development.
- Automatically convert every existing Flutter, React Native or Qt application.

---

# 3. Target Platforms

The initial production scope prioritizes six core deployment targets (Android, iOS, Windows, macOS, Linux, and Web). The architecture is intentionally decoupled to accommodate extended targets (wearables, TV, spatial computing, and embedded/automotive platforms) in subsequent phases without breaking core engine abstractions.

## 3.1 Android

Requirements:

- Android application packaging.
- Android NDK integration (C/C++ runtime execution and GPU surface access).
- JNI interoperability and Java/Kotlin framework bridge (mandatory for Android SDK framework APIs including permissions, notifications, telephony, audio routing, background work, and device sensors).
- Native Activity / custom SurfaceView lifecycle integration.
- Touch, multi-touch, and gesture recognition.
- Camera and microphone access.
- GPS and location APIs.
- Notifications and FCM integration.
- Background execution and work manager scheduling.
- File system and scoped storage integration.
- Bluetooth LE integration.
- Accessibility bridge (mapping custom display list semantics to Android `AccessibilityNodeInfo` hierarchy).
- Android lifecycle management (onPause, onResume, onStop, onDestroy, configuration changes).
- APK and AAB generation.

Build output:

- APK (Debug and Release packages)
- AAB (Android App Bundle for Google Play distribution)
- Native shared libraries (`.so`)

## 3.2 iOS

Requirements:

- Xcode integration and toolchain pipeline.
- Apple SDK and Metal graphics integration.
- Native application lifecycle (UIApplicationDelegate and scene lifecycle).
- Objective-C++ / Swift interoperability bridge for Apple framework APIs.
- Touch and gesture recognition.
- Camera, audio session, and microphone.
- CoreLocation services.
- APNs push notifications.
- Background execution where permitted by Apple guidelines.
- Keychain integration and secure enclave access.
- Accessibility bridge (mapping custom display list semantics to `UIAccessibilityElement` tree).
- Apple signing, provisioning profile management, and notarization.
- App Store packaging and bitcode/symbols management.

Build output:

- `.app` application bundle (compiled native bundle).
- `.ipa` distribution package (signed App Store / Ad-Hoc / TestFlight package).

Important: iOS release builds require Apple's supported toolchain and signing environment, normally running on macOS.

## 3.3 Windows

Requirements:

- Windows 10 and Windows 11 support (x86_64 and ARM64).
- Native window management (Win32 / C++/WinRT).
- Keyboard, mouse, and high-precision scrolling.
- Multi-window and child window applications.
- System tray, taskbar integration, and jump lists.
- Clipboard and drag-and-drop.
- Native file and folder dialogs.
- Toast notifications and action center.
- Native filesystem and path abstractions.
- Direct3D 12 and Vulkan GPU rendering.
- UI Automation / Windows accessibility APIs bridge.

Build output:

- Portable executable (`.exe`).
- Shared runtime libraries (`.dll`).
- MSIX installer package and WiX MSI installer.

## 3.4 macOS

Requirements:

- Apple Silicon (ARM64) and Intel (x86_64) Universal 2 binaries.
- Native window management and AppKit integration.
- Native macOS menu bar and contextual menus.
- Dock integration and badge management.
- Keyboard, mouse, and trackpad multi-touch gestures (pinch, rotate, swipe).
- macOS notifications.
- Native open/save file dialogs and sandbox entitlements.
- Keychain integration.
- Retina / high-DPI rendering via Metal.
- NSAccessibility bridge for VoiceOver and assistive tech.
- Hardened runtime, codesigning, and notarization.

Build output:

- `.app` bundle.
- `.dmg` disk image installer.
- `.pkg` installer package.

## 3.5 Linux

Requirements:

- Support for major distributions (Ubuntu, Debian, Fedora, Arch, RHEL).
- X11 and Wayland display server backends.
- Native window management and client-side/server-side decoration support.
- File system and XDG desktop standards compliance.
- Keyboard, mouse, touchpad, and input method editor (IME) support.
- GPU acceleration via Vulkan and OpenGL fallback.
- Desktop notifications (FreeDesktop standard).
- AT-SPI2 accessibility bus integration.

Build output:

- ELF executable.
- AppImage (standalone cross-distro package).
- Flatpak package.
- Distribution packages (`.deb`, `.rpm`).

## 3.6 Web

Requirements:

- WebAssembly (WASM) compilation via Emscripten / LLVM.
- Browser execution within modern web environments.
- Canvas rendering pipeline with dual backends:
  - WebGPU acceleration where supported by browser and hardware.
  - WebGL 2.0 / WebGL 1.0 automatic fallback.
  - Software rasterizer fallback for headless testing and unsupported environments.
- Browser DOM event handling, touch events, and keyboard mapping.
- Responsive layout and dynamic window/viewport resizing.
- Browser storage (IndexedDB, LocalStorage, Cache API).
- HTTP, Fetch API, and WebSocket client support.
- Web Accessibility (ARIA live regions and semantic DOM overlay synchronization).
- Progressive Web Application (PWA) manifest and service worker integration.

Build output:

- HTML host page (`index.html`).
- JavaScript loader and glue code.
- WebAssembly module binaries (`.wasm`).
- Asset bundle and resource manifests.

Web limitations and unsupported browser capabilities must be documented clearly.

## 3.7 watchOS (Extended Target – Future Phase)

Requirements:

- WatchKit and SwiftUI / native hosting adapter.
- Low-power, ultra-compact memory footprint and aggressive CPU throttling adaptations.
- Digital Crown input and wrist gestures.
- HealthKit, workout, and sensor streaming.
- Watch complications and Glance/Smart Stack interfaces.
- iOS companion app data synchronization (Watch Connectivity framework).
- Standalone watchOS app packaging and App Store distribution.

Build output:

- watchOS `.app` bundle.
- Watch companion archive embedded in iOS `.app`.

## 3.8 Wear OS (Extended Target – Future Phase)

Requirements:

- Android Wear OS SDK and Wearable Support Library integration.
- Round and square screen layout adaptations.
- Rotary input (crown, rotating bezel) and quick swipe dismiss gestures.
- Tiles API and Ongoing Activity notifications.
- Health Services API and low-power ambient mode.
- Bluetooth / Cloud sync with paired Android smartphone.
- Play Store Wear OS packaging.

Build output:

- Wear OS APK / AAB.

## 3.9 tvOS (Extended Target – Future Phase)

Requirements:

- Apple TV SDK integration with Metal rendering.
- Siri Remote interaction (touch surface, clickpad, D-pad, and motion gestures).
- Directional focus engine (focus navigation between widgets, focus effects, and parallax).
- Top Shelf content provider extensions.
- TV-optimized 10-foot user experience layouts (overscan protection, high contrast).
- In-app purchases and Apple TV Store packaging.

Build output:

- tvOS `.app` bundle.
- tvOS distribution package.

## 3.10 Android TV / Google TV (Extended Target – Future Phase)

Requirements:

- Android Leanback / Android TV SDK integration.
- D-Pad remote control navigation and focus highlight state engine.
- TV home screen recommendations and Channels / Watch Next integration.
- 10-foot UI design guidelines with TV overscan margins and large-format typography.
- Media session and remote playback controllers.
- Android TV Play Store packaging.

Build output:

- Android TV compatible APK / AAB.

## 3.11 visionOS (Extended Target – Future Phase)

Requirements:

- RealityKit and spatial rendering pipeline integration.
- Spatial audio, eye tracking, and hand gesture input recognition.
- Windowed (2D planar in 3D space), Volumetric (3D bounded boxes), and Immersive Space execution modes.
- Pass-through interaction and dynamic lighting adaptation.
- visionOS SDK and App Store packaging.

Build output:

- visionOS `.app` bundle.
- visionOS distribution package.

## 3.12 Embedded Linux & QNX (Extended Target – Future Phase)

Requirements:

- Real-time operating system (RTOS) compatibility (QNX Neutrino RTOS 7.x/8.x).
- Embedded Linux via Yocto Project / Buildroot toolchains.
- Direct-to-display rendering (Linux DRM/KMS, Wayland-IVI, QNX Screen graphics subsystem).
- Strict deterministic memory management, zero-garbage collection, and fault-tolerant startup.
- Automotive CAN bus, industrial serial protocols, and vehicle telemetry integration.
- Headless, single-process, and multi-display digital cockpit / instrument cluster profiles.

Build output:

- Cross-compiled stripped native binaries.
- Embedded system image components / dynamic shared objects.

---

# 4. High-Level System Architecture

The framework will use a layered architecture.

## 4.1 Architecture Overview

```text
                             DEVELOPER APPLICATION
                                       |
                                       |
                              PINGDART SDK / API
                                       |
        +------------------------------+------------------------------+
        |                              |                              |
   UI FRAMEWORK                   APP SERVICES                DOMAIN MODULES
        |                              |                              |
  Widget System                  Networking                  PingDart.Wearable
  Layout Engine                  Storage / DB                PingDart.TV
  Navigation                     State Management            PingDart.Spatial
  Animation                      Permissions                 PingDart.Automotive
  Themes                         Task Scheduling             PingDart.Embedded
        |                              |                              |
        +------------------------------+------------------------------+
                                       |
                             FOUNDATIONAL MODULES
                  +--------------------+--------------------+
                  |                                         |
         PingDart.Accessibility                      PingDart.Device
                  |                                         |
                  +--------------------+--------------------+
                                       |
                                 PINGDART CORE
                                       |
                  +--------------------+--------------------+
                  |                    |                    |
             Runtime Engine     Rendering Engine       Event Engine
             (Tri-Modal)       (Custom/Native/Hybrid)  (Loop & Dispatch)
                  |                    |                    |
                  +--------------------+--------------------+
                                       |
                             PLATFORM ABSTRACTION
                                       |
       +---------+----------+----------+----------+---------+---------+
       |         |          |          |          |         |         |
    Android     iOS      Windows     macOS      Linux      Web    Embedded/RTOS
       |         |                                          |      (QNX/Linux)
   Wear OS    watchOS                                      tvOS
      &          &                                          &
  Android TV  visionOS                                  Google TV
```

## 4.2 Architecture Layers

### Layer 1: Developer Application

Contains application-specific business logic, screens, and UI components.

### Layer 2: SDK & Specialized Domain Modules

Provides unified public APIs for application development, extended by modular domain libraries (`PingDart.Wearable`, `PingDart.TV`, `PingDart.Spatial`, etc.).

### Layer 3: UI Framework

Provides the declarative widget tree, layout solvers, navigation graphs, animation controllers, and theme styling.

### Layer 4: Foundation Services (Accessibility & Device)

Manages cross-platform assistive technology bridges and low-level hardware abstraction across all devices.

### Layer 5: Core Runtime & Tri-Modal Rendering Engine

Handles application lifecycle, event loops, resource management, and flexible rendering execution (Custom, Native, or Hybrid).

### Layer 6: Platform Abstraction Layer (PAL)

Connects framework primitives to operating-system native interfaces via clean C++ abstraction boundaries.

### Layer 7: Platform Backends & Native Adapters

Contains target-specific implementations for each supported operating system, bridging native host windows, platform views, and platform SDK APIs.

## 4.3 Additional Architectural Modules

To support multi-form-factor devices without bloating the core runtime, PingDart structures specialized capabilities into modular components:

| Module | Responsibility |
|---|---|
| **PingDart.Wearable** | Smartwatch APIs, circular screen layout helpers, rotary input (crown/bezel) listeners, ambient low-power display modes, watch complications, and health sensor streaming. |
| **PingDart.TV** | 10-foot UI layout rules, remote control navigation (D-Pad), directional focus engine with parallax/glow highlights, overscan margins, and Top Shelf / channel feeds. |
| **PingDart.Spatial** | Spatial computing and 3D UI interfaces, planar-in-space windowing, volumetric rendering bounds, gesture/eye-tracking input, and spatial audio positioning. |
| **PingDart.Automotive** | Automotive integrations, vehicle telemetry abstraction, CAN bus and OBD data adapters, digital cluster profiles, multi-display routing, and driver distraction guidelines. |
| **PingDart.Embedded** | Embedded hardware interfaces (GPIO, I2C, SPI), direct DRM/KMS framebuffer rendering, zero-copy display pipelines, and RTOS task integration (QNX/FreeRTOS). |
| **PingDart.Accessibility** | Universal accessibility tree builder, semantic node dispatch, and platform bridge mapping to Windows UI Automation, macOS NSAccessibility, Linux AT-SPI2, Android AccessibilityNodeInfo, iOS UIAccessibility, and Web ARIA. |
| **PingDart.Device** | Unified sensors and hardware abstraction layer (accelerometer, gyroscope, compass, barometer, biometric auth, camera session management, GPS, and battery states). |

---

# 5. Programming Language Strategy

## 5.1 Initial Language

The initial framework API will use modern C++.

Recommended baseline:

- C++20 minimum.
- C++23 adoption where toolchain support is stable.
- RAII-based resource management.
- Strong typing.
- Modern templates.
- Asynchronous programming.
- Controlled memory allocation.

## 5.2 Public API

Developers should be able to write readable code.

Example:

```cpp
#include <pingdart/pingdart.h>

using namespace pingdart;

class HomePage : public Widget {
public:

    Widget build() override {

        return Scaffold({

            AppBar("PingDart"),

            Column({

                Text("Welcome"),

                Button("Click Me", [] {
                    print("Button clicked");
                }),

                TextField()
            })
        });
    }
};

int main() {

    App app("MyApplication");

    app.home<HomePage>();

    return app.run();
}
```

This is a proposed API and must be finalized during SDK design.

## 5.3 Future Language Support

The framework should be designed to allow a dedicated PingDart language in future versions.

Possible syntax:

```text
app MyApplication

screen HomePage:

    Column:
        Text("Welcome to PingDart")

        Button("Click Me"):
            onClick:
                print("Clicked")
```

The dedicated language must not be required for the initial release.

---

# 6. Compiler Architecture

## 6.1 Compiler Responsibilities

The compiler system must support:

- Source parsing.
- Semantic analysis.
- Type checking.
- Module resolution.
- Intermediate representation.
- Optimization.
- Platform-specific code generation.
- Debug symbols.
- Error reporting.
- Source maps where applicable.

## 6.2 Compiler Components

```text
Source Code
     |
Lexer
     |
Parser
     |
AST
     |
Semantic Analyzer
     |
Intermediate Representation
     |
Optimization
     |
Platform Code Generation
     |
Application Package
```

## 6.3 LLVM Integration

Use LLVM and Clang where appropriate.

Responsibilities:

- C++ compilation.
- Optimization.
- Target-specific code generation.
- Debugging support.
- Cross-compilation integration.

Do not unnecessarily build a replacement for an existing production-quality C++ compiler.

## 6.4 PingDartCore Integration

Evaluate the existing PingDartCore Rust compiler as a reusable subsystem.

Requirements:

- Preserve existing stable compiler functionality.
- Establish a documented interface between Rust compiler components and the C++ framework.
- Avoid duplicate compiler implementations.
- Separate language parsing from UI runtime functionality.
- Support independent versioning.
- Provide reproducible builds.

Integration must be validated with existing PingDartCore test suites before adoption.

---

# 7. Rendering Engine

The rendering engine is one of the most important components.

## 7.1 Requirements

- High-performance 2D rendering.
- Text rendering.
- Image rendering.
- Vector graphics.
- Gradients.
- Shadows.
- Clipping.
- Opacity.
- Transformations.
- Layers.
- Compositing.
- Animation.
- GPU acceleration.
- High-DPI displays.

## 7.2 Graphics Backend

Use an abstraction layer with dynamic fallback support:

| Backend | Target | Fallback Strategy |
|---|---|---|
| Vulkan | Android, Linux, supported Windows | OpenGL ES / OpenGL 3.3+ |
| Metal | iOS, macOS, tvOS, visionOS | Software rasterizer (testing only) |
| Direct3D 12 | Windows 10/11 | Direct3D 11 / WARP Software |
| WebGPU | Web (modern browsers) | WebGL 2.0 -> WebGL 1.0 -> CPU Canvas |
| OpenGL ES | Android fallback, Embedded Linux | Software renderer |
| DRM/KMS / QNX Screen | Embedded Linux, QNX | Direct Framebuffer |
| Software renderer | Headless CI, unit testing, failover | N/A |

*WebGPU Note:* Because WebGPU availability varies across user hardware and browser implementations, WebGL 2.0 and CPU rasterization fallbacks are mandatory for robust production delivery.

Evaluate Skia as an optional rendering foundation rather than immediately implementing every graphics primitive from scratch.

## 7.3 Rendering Pipeline

```text
Widget Tree
    |
Layout Calculation
    |
Paint Command Generation
    |
Display List
    |
Render Graph & Compositor
    |
GPU Backend (Vulkan / Metal / D3D12 / WebGPU)
    |
Platform View Composition / Overlay
    |
Screen Output
```

## 7.4 Tri-Modal Rendering Architecture

To deliver maximum flexibility across mobile phones, desktop workstations, televisions, spatial goggles, and smartwatches, PingDart adopts a **Tri-Modal Rendering Strategy**:

1. **Custom Rendering Mode (Consistent UI):**
   - High-performance, pixel-exact rendering driven directly by the PingDart rendering pipeline to a GPU surface (similar to Flutter or game engines).
   - Guarantees identical look-and-feel across all platforms, 60/120 FPS animations, and complete control over custom design systems.
   - *Architecture Warning:* Custom rendering does not automatically provide native accessibility trees, native system menus, or native controls. These must be bridged explicitly via dedicated platform adapters (e.g., exposing semantic nodes to accessibility daemons).

2. **Native Rendering Mode (Platform Controls):**
   - Directly instantiates and orchestrates platform-native widgets (e.g., WinUI/Win32 controls on Windows, AppKit controls on macOS, UIKit on iOS, Android Views on Android).
   - Essential for strict platform adherence, out-of-the-box system accessibility, native text selection, and form autofill integrations.

3. **Hybrid Rendering Mode (Composed UI):**
   - Seamlessly blends custom PingDart canvas rendering with embedded native platform views (e.g., embedding a native Google Map, Apple Map, WebKit WebView, native camera preview, or payment sheet inside a custom widget hierarchy).
   - Utilizes virtual display composition, texture sharing, or hybrid layer stacking without noticeable frame drop.

## 7.5 Performance Requirements

Initial targets:

- 60 FPS for standard animations.
- 120 FPS where hardware and display support permit.
- Minimal unnecessary widget rebuilding.
- Efficient image caching.
- Efficient text layout caching.
- GPU resource management.
- Reduced rendering allocations.

Targets must be measured on defined reference devices.

---

# 8. Widget System

The framework must provide a comprehensive widget library.

## 8.1 Basic Widgets

- Text
- RichText
- Button
- Icon
- Image
- Avatar
- Divider
- ProgressIndicator
- Checkbox
- RadioButton
- Switch
- Slider
- TextField
- TextArea

## 8.2 Layout Widgets

- Row
- Column
- Stack
- Container
- Padding
- Center
- Align
- Expanded
- Flexible
- Grid
- ListView
- ScrollView
- Wrap
- Positioned

## 8.3 Navigation

- Navigator
- Router
- Page
- TabBar
- BottomNavigation
- Drawer
- Dialog
- BottomSheet
- Modal

## 8.4 Advanced Widgets

- DataTable
- Calendar
- Charts
- VideoPlayer
- AudioPlayer
- CameraView
- MapView
- WebView
- FilePicker
- RichTextEditor

Advanced widgets may use native platform integrations.

---

# 9. Layout Engine

The layout system must support:

- Flexbox-like layouts.
- Grid layouts.
- Absolute positioning.
- Intrinsic sizing.
- Percentage sizing.
- Minimum and maximum constraints.
- Responsive layouts.
- Scrollable content.
- Safe areas.
- Dynamic screen sizes.

## 9.1 Layout Example

```cpp
Column({
    Text("Dashboard"),

    Row({
        Expanded(Card()),
        Expanded(Card())
    }),

    Grid({
        GridItem(),
        GridItem(),
        GridItem(),
        GridItem()
    })
});
```

The layout engine must be deterministic and independently testable.

---

# 10. State Management

Provide a standard state management API.

Required capabilities:

- Local state.
- Shared application state.
- Reactive updates.
- Immutable state options.
- Lifecycle-aware state.
- Event-driven state.
- Async state.
- State persistence.

Example:

```cpp
class Counter : public StatefulWidget {

    int count = 0;

    Widget build() override {

        return Column({

            Text(std::to_string(count)),

            Button("Increment", [&] {
                setState([&] {
                    count++;
                });
            })
        });
    }
};
```

The API must prevent unsafe references and callbacks from outliving their owning components.

---

# 11. Animation Engine

Support:

- Implicit animation.
- Explicit animation.
- Timelines.
- Curves.
- Transitions.
- Gesture-based animation.
- Physics-based motion.
- Frame scheduling.

Required examples:

- Fade.
- Slide.
- Scale.
- Rotation.
- Hero-style transitions.
- Page transitions.
- Custom animation controllers.

---

# 12. Native Platform Integration

Each operating system needs a dedicated adapter.

## 12.1 Android Adapter

Technology:

- C++.
- Android NDK.
- JNI.
- Android SDK integration.

Must support:

- Application lifecycle.
- Activity integration.
- Permissions.
- Notifications.
- Camera.
- Microphone.
- Sensors.
- Location.
- Background execution.
- Android packaging.

## 12.2 iOS Adapter

Technology:

- C++.
- Objective-C++.
- Swift interoperability.
- Apple SDK.

Must support:

- UIKit integration.
- Application lifecycle.
- Permissions.
- Notifications.
- Camera.
- Sensors.
- Keychain.
- Native dialogs.

## 12.3 Windows Adapter

Technology:

- C++.
- Win32.
- C++/WinRT where appropriate.

Must support:

- Window management.
- System tray.
- Notifications.
- Clipboard.
- Native dialogs.
- File system.
- Accessibility.

## 12.4 macOS Adapter

Technology:

- C++.
- Objective-C++.
- AppKit integration.

Must support:

- Native window management.
- Menu bar.
- Dock.
- Notifications.
- File dialogs.
- Keychain.
- Trackpad gestures.

## 12.5 Linux Adapter

Technology:

- C++.
- X11.
- Wayland.

Must support:

- Window management.
- Input.
- Display management.
- Clipboard.
- Notifications.
- File dialogs.

## 12.6 Web Adapter

Technology:

- C++.
- WebAssembly.
- JavaScript interoperability.
- WebGPU/WebGL.

Must support:

- Browser lifecycle.
- Canvas.
- Input events.
- Browser storage.
- Networking.
- Browser navigation.
- Responsive rendering.

## 12.7 Runtime Capability Detection

Applications must never rely on brittle compile-time `#ifdef` checks or raw OS version strings alone to infer feature availability. PingDart provides a centralized runtime capability discovery API based on a **Four-Tier Capability Evaluation Model**:

1. **Tier 1 – Hardware Availability:** Physical device presence (e.g., does the device possess an autofocus camera sensor, biometric scanner, accelerometer, or rotary dial?).
2. **Tier 2 – Operating System Support:** OS API presence (e.g., does the host OS version expose the corresponding SDK APIs or HAL drivers to interact with that hardware?).
3. **Tier 3 – Permission & Policy Status:** User & system authorization (e.g., has the user or enterprise device policy granted active runtime permission to access the resource?).
4. **Tier 4 – Runtime Readiness & State:** Immediate resource availability (e.g., is the hardware resource currently unlocked and uninhibited, or is the camera/microphone held exclusively by another application or in low-power suspension?).

```cpp
// Explicit 4-tier capability query:
CapabilityState state = Platform::query_capability(Capability::Camera);
if (state.is_ready()) {
    camera.start_preview();
} else if (state.needs_permission()) {
    Platform::request_permission(Capability::Camera, on_granted_callback);
} else if (state.is_busy()) {
    show_ui_notice("Camera is currently in use by another application.");
}
```

## 12.8 Real-Time Isolation Architecture (Embedded & QNX)

Real-time embedded systems (automotive digital clusters, industrial HMIs) cannot share the same execution assumptions as general desktop or mobile applications:

1. **Isolation from Desktop/Mobile Runtime:**
   - The QNX Neutrino and Embedded Linux profile must exist as an isolated, modular runtime subsystem rather than sharing a monolithic binary with desktop/mobile code.

2. **Real-Time Determinism Disclaimer & Guarantees:**
   - *Critical Architecture Rule:* **Deterministic execution is NOT guaranteed merely by using C++ or avoiding garbage collection.**
   - Hard real-time constraints require:
     - Preallocated memory arenas with zero dynamic heap allocations (`malloc`/`new`) on the render/critical execution threads.
     - Lock-free, bounded ring buffers and wait-free queues between the UI event loop and hardware telemetry.
     - Strict priority inheritance mutexes and RTOS real-time scheduling policies (`SCHED_FIFO` / `SCHED_RR`).
     - Hard isolation between high-frequency automotive/sensor telemetry threads and visual rendering passes.

---

# 13. Networking Module

The framework must include a common networking API.

Requirements:

- HTTP.
- HTTPS.
- REST.
- WebSocket.
- Streaming.
- TLS.
- Request cancellation.
- Timeouts.
- Retry policies.
- Authentication.
- Proxy support.

Example:

```cpp
HttpClient client;

auto response = client.get(
    "https://api.example.com/users"
);
```

Use a platform-independent networking abstraction, with TLS and certificate validation handled securely.

---

# 14. Storage and Database

Provide standardized storage APIs.

## 14.1 Storage

- Local files.
- Application cache.
- Secure storage.
- Preferences.
- Asset management.

## 14.2 Database

Support:

- SQLite.
- Optional embedded databases.
- Remote databases through application APIs.
- Transactions.
- Asynchronous database operations.

Example:

```cpp
Database db("app.db");

db.execute(
    "CREATE TABLE IF NOT EXISTS users (id INTEGER)"
);
```

Sensitive information must use secure storage rather than ordinary preferences.

---

# 15. Security Architecture

Security is mandatory.

Requirements:

- TLS by default.
- Certificate validation.
- Secure secret handling.
- Memory safety.
- Input validation.
- Plugin permissions.
- Dependency integrity.
- Secure package signing.
- Code signing support.
- No hardcoded credentials.
- Sandboxed execution where supported.
- Secure update verification.

The framework must never claim that compiled applications are impossible to reverse engineer.

---

# 16. Plugin and Package Ecosystem

Build an independent package ecosystem.

## 16.1 Package Manager

Proposed CLI:

```bash
pd package add http
pd package add database
pd package add camera
pd package remove camera
pd package update
pd package publish
```

## 16.2 Plugin Architecture

Plugin types:

- Native C++ plugins.
- Platform-specific plugins.
- UI component packages.
- Networking plugins.
- Storage plugins.
- Third-party service integrations.

Each plugin must include:

- Package metadata.
- Version.
- Platform compatibility.
- Dependencies.
- Permissions.
- License.
- Documentation.
- Integrity information.

## 16.3 Plugin Isolation and Security Architecture

Plugins must not receive unrestricted access automatically:

1. **Permission Declarations & Manifests:**
   - Every plugin package must declare required capabilities in a `plugin.yaml` manifest (e.g., `permissions: [network.http, device.camera, storage.cache]`).
   - The host application verifies and grants permissions during compilation or runtime initialization. Attempted unauthorized platform calls fail safely with runtime exceptions.

2. **Plugin Lifecycle Management:**
   - Formal lifecycle contracts: `on_init()`, `on_activate()`, `on_suspend()`, `on_resume()`, `on_deactivate()`, and `on_destroy()`.
   - Host applications can gracefully unload, restart, or failover individual plugin instances without crashing the core engine.

3. **Semantic Versioning & Compatibility Matrix:**
   - Strict version resolution prevents ABI incompatibilities across PingDart SDK releases.
   - Built-in checks verify C++ standard library / compiler toolchain compatibility before linking.

4. **Crash Isolation & Process Sandboxing:**
   - *Security Note on Native Plugins:* Operating system signal handlers (POSIX `SIGSEGV`, `SIGBUS`) and Windows Structured Exception Handling (SEH) cannot reliably guarantee state containment in the presence of arbitrary native undefined behavior, stack corruption, or heap poisoning.
   - For trusted internal plugins, in-process defensive wrappers are provided for diagnostics and cleanup.
   - For all untrusted or third-party native plugins, PingDart enforces **out-of-process isolation** (sandboxed child process communicating with the host via high-throughput shared memory / IPC), ensuring that an isolated plugin crash never crashes the main host application.

---

# 17. Developer CLI

Create an official CLI named `pd`.

## 17.1 Commands

```bash
pd create myapp
pd init
pd run android
pd run ios
pd run windows
pd run macos
pd run linux
pd run web
pd build android
pd build ios
pd build windows
pd build macos
pd build linux
pd build web
pd test
pd analyze
pd clean
pd doctor
pd package add
pd package publish
```

## 17.2 CLI Requirements

- Cross-platform installation.
- Project templates.
- Dependency management.
- Build configuration.
- Platform detection.
- SDK verification.
- Error diagnostics.
- Build caching.
- Parallel builds.
- CI/CD support.

---

# 18. Project Structure

Recommended framework repository:

```text
PingDartFramework/
│
├── engine/
│   ├── core/
│   ├── runtime/
│   ├── memory/
│   ├── events/
│   ├── scheduler/
│   └── lifecycle/
│
├── rendering/
│   ├── display_list/
│   ├── text/
│   ├── images/
│   ├── compositor/
│   ├── gpu/
│   └── software/
│
├── ui/
│   ├── widgets/
│   ├── layouts/
│   ├── navigation/
│   ├── animation/
│   ├── themes/
│   └── accessibility/
│
├── platform/
│   ├── android/
│   ├── ios/
│   ├── windows/
│   ├── macos/
│   ├── linux/
│   └── web/
│
├── compiler/
│   ├── frontend/
│   ├── ir/
│   ├── optimizer/
│   └── backend/
│
├── sdk/
│   ├── include/
│   ├── libraries/
│   └── templates/
│
├── cli/
│
├── package_manager/
│
├── devtools/
│
├── examples/
│
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── rendering/
│   └── platform/
│
├── docs/
│
├── third_party/
│
├── CMakeLists.txt
│
└── README.md
```

---

# 19. Build System & Cross-Compilation Architecture

Use CMake (3.26+) and Ninja as the unified foundational build engine.

Recommended supporting technologies:

- CMake & Ninja.
- LLVM / Clang (primary cross-compilation driver).
- Android NDK (r25+).
- Apple Xcode Command Line Tools.
- Windows 10/11 SDK & MSVC toolchain.
- Emscripten SDK (emsdk) for WebAssembly.

Requirements:

- Incremental builds.
- Cross-compilation across all target platforms.
- Dependency caching and ccache integration.
- Reproducible, bit-identical release builds.
- Parallel compilation.
- Platform-specific packaging automation.
- Debug, Profile, and Release configurations.

The `pd` CLI should hide most CMake complexity from application developers.

## 19.1 Cross-Compilation & Toolchain System

Building for multiple operating systems from developer workstations requires a strictly managed cross-compilation architecture:

1. **Host SDK & Toolchain Discovery:**
   - Automated environment detection via `pd doctor` for host SDKs (Xcode SDK paths on macOS, `ANDROID_HOME` / `ANDROID_NDK_ROOT`, Windows 10/11 Kits registry entries, Emscripten paths).
   - Pre-packaged CMake toolchain files (`cmake/toolchains/android.toolchain.cmake`, `ios.toolchain.cmake`, `wasm.toolchain.cmake`, etc.).

2. **Hermetic Dependency Management:**
   - External dependencies (e.g., Skia, HarfBuzz, FreeType, SQLite, libpng) must be locked by exact cryptographic hash using CMake `FetchContent` or a dedicated package cache (vcpkg / Conan).
   - Zero undeclared system dependencies allowed in production release builds.

3. **Automated Signing Pipelines:**
   - Apple Codesigning, Provisioning Profile embedding, and automated Apple Notarization CLI integration (`xcrun notarytool`).
   - Android keystore management, `zipalign`, and `apksigner` integration.
   - Windows Authenticode digital signing (`signtool.exe`) with Hardware Security Module (HSM) and EV certificate support.

4. **Reproducible Builds:**
   - Path-prefix remapping (`-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.`) to ensure binaries built on different developer machines or CI agents produce bit-identical hashes.
   - Deterministic timestamp stripping from compiled binary headers.

---

# 20. Hot Reload and Developer Tools

A modern framework needs excellent developer tooling.

Required features:

- Hot reload.
- Hot restart.
- UI inspector.
- Widget tree inspector.
- Layout inspector.
- Performance profiler.
- Memory profiler.
- Network inspector.
- Logs.
- Breakpoints.
- Stack traces.
- Crash reporting integration.

Hot reload must preserve compatible application state where possible.

Native code changes may require recompilation and application restart.

---

# 21. Accessibility and Internationalization

Accessibility is a core requirement, not an optional extension.

Support:

- Screen readers.
- Keyboard navigation.
- Focus management.
- High contrast.
- Dynamic text scaling.
- Semantic labels.
- Accessible controls.
- RTL layouts.
- Unicode.
- International fonts.
- Locale-aware formatting.

The custom renderer must expose semantic information to platform accessibility APIs.

---

# 22. Testing Strategy

## 22.1 Unit Testing

Test:

- Core runtime.
- Memory management.
- Layout calculations.
- State management.
- Rendering commands.
- Navigation.
- Networking.

## 22.2 Integration Testing

Test:

- Native APIs.
- Platform lifecycle.
- Plugin integration.
- Storage.
- Camera.
- Notifications.
- Build and packaging.

## 22.3 Rendering & Cross-Platform Conformance Testing

The framework must guarantee visual, structural, and semantic fidelity across all operating systems through automated conformance suites:

1. **Pixel Golden Tests & GPU Parity:**
   - Automated screenshot capture comparing rendered UI components against baseline pixel golden masters.
   - Cross-GPU validation: The identical widget tree rendered via Vulkan, Metal, Direct3D 12, and WebGPU is evaluated using Structural Similarity (SSIM). SSIM thresholds (e.g. baseline target SSIM > 0.995) are configurable per test suite to accommodate legitimate cross-platform variance stemming from font subpixel rasterizers (FreeType vs. DirectWrite vs. CoreText), GPU driver floating-point precision, and display color management profiles.

2. **Text Shaping & Typography Conformance:**
   - Complex script shaping verification (Arabic, Devanagari, Hebrew, CJK) via HarfBuzz font engine.
   - Bi-directional (BiDi) text layout and fallback font cascade correctness across all platforms.

3. **Layout & Multi-DPI Scaling Conformance:**
   - Testing flex, grid, and constraint calculation consistency across screen densities (1x, 1.25x, 1.5x, 2x, 3x Retina/HiDPI).
   - Layout reflow validation across orientation shifts, foldables, and dynamic window resizing.

4. **Cross-Platform Accessibility Conformance:**
   - Semantic tree inspection ensuring that widgets generate equivalent accessibility roles, values, and actions across Windows UI Automation, macOS NSAccessibility, Linux AT-SPI2, Android AccessibilityNodeInfo, and iOS UIAccessibility.

## 22.4 Performance Testing

Measure:

- Startup time.
- Frame rendering time.
- Memory usage.
- CPU usage.
- GPU usage.
- Scrolling performance.
- Application package size.
- Build duration.

## 22.5 Compatibility Testing

Test across:

- Android device variations.
- iOS device variations.
- Windows versions.
- macOS versions.
- Linux distributions.
- Major web browsers.

---

# 23. Performance Targets

These are engineering targets, not guaranteed results.

| Metric | Initial target |
|---|---|
| Standard animation | 60 FPS |
| High refresh display | 120 FPS where supported |
| UI responsiveness | No unnecessary main-thread blocking |
| Memory management | Bounded and measurable allocations |
| Build system | Incremental compilation |
| Networking | Asynchronous operations |
| UI updates | Efficient partial updates |
| Startup | Measured against reference hardware |

Define specific baseline hardware and benchmark procedures before accepting performance claims.

---

# 24. CI/CD and Release Management

Implement automated pipelines for:

- Linux builds.
- Windows builds.
- macOS builds.
- Android builds.
- iOS builds.
- Web builds.

Requirements:

- Automated compilation.
- Unit tests.
- Integration tests.
- Static analysis.
- Security scanning.
- Dependency scanning.
- Artifact generation.
- Release signing.
- Version management.
- Regression testing.

Use a release channel strategy:

- Nightly.
- Development.
- Beta.
- Stable.
- Long-term support.

---

# 25. Development Roadmap

The engineering roadmap follows a staged, disciplined rollout. The initial production scope remains strictly focused on six primary platforms (Phases 1 through 5), while the underlying runtime and abstraction layer are intentionally designed to accommodate extended form factors (Phases 6 through 9) without requiring breaking rewrites.

```text
Phase 1: Windows, Linux (Core Desktop Runtime)
    |
Phase 2: Android (Mobile NDK + JNI Framework Bridge)
    |
Phase 3: macOS (Desktop Universal2 + AppKit + Metal)
    |
Phase 4: iOS (Mobile UIKit + Metal + Xcode Pipeline)
    |
Phase 5: Web (WebAssembly + WebGPU + WebGL Fallback)
    |  ===> INITIAL PRODUCTION RELEASE MILESTONE (6 Core Platforms)
Phase 6: Android TV & Google TV (Leanback + D-Pad Focus Engine)
    |
Phase 7: watchOS & Wear OS (Low-Power Wearables + Rotary Input)
    |
Phase 8: tvOS & visionOS (Siri Remote 10-ft UI & Spatial Computing)
    |
Phase 9: Embedded Linux & QNX (Automotive & Real-Time Industrial Systems)
```

## Phase 0 – Architecture, Research & POC
- Finalize technical architecture and Tri-Modal rendering specifications.
- Evaluate PingDartCore Rust compiler integration and C++20 engine boundaries.
- Graphics backend abstraction layer design (Skia evaluation, Vulkan/Metal/D3D12/WebGPU).
- Exit criteria: Approved architecture document and working windowed triangle proof-of-concept.

## Phase 1 – Windows & Linux (Desktop Core Runtime)
- Core runtime engine, event loop, memory allocators, and scheduler.
- Windows (Win32 / C++/WinRT) and Linux (X11 / Wayland) platform adapters.
- Custom rendering pipeline (Vulkan & Direct3D 12 backends).
- Basic layout engine (Row, Column, Stack) and basic widgets (Text, Button).
- Exit criteria: A standalone desktop application running stably on Windows 11 and Ubuntu Linux.

## Phase 2 – Android (Mobile NDK & JNI Framework Bridge)
- Android NDK runtime execution and SurfaceView graphics attachment.
- JNI framework bridge for platform capabilities (permissions, lifecycle, FCM notifications).
- Touch and multi-touch gesture dispatch.
- Android AccessibilityNodeInfo bridge.
- Gradle build plugin and APK/AAB generation.
- Exit criteria: A production-ready Android test application running on physical Android devices.

## Phase 3 – macOS (Universal2 & Desktop AppKit)
- macOS platform adapter with AppKit host window.
- Metal GPU graphics pipeline for macOS.
- Native macOS menu bar, contextual menus, and Dock badge integration.
- Trackpad gesture recognition and Retina high-DPI scaling.
- Universal2 (Apple Silicon ARM64 + Intel x86_64) `.app` bundle, `.dmg`, and `.pkg` packaging.
- Exit criteria: Identical sample project compiles and runs with native macOS menus on Apple Silicon.

## Phase 4 – iOS (Mobile UIKit & Metal Pipeline)
- iOS platform adapter with UIWindow/UIView hosting and Metal backend.
- Objective-C++ / Swift interoperability bridge for Apple framework APIs.
- Touch/gesture recognition and keyboard input management.
- APNs push notifications and background execution modes.
- `UIAccessibilityElement` tree bridge for VoiceOver.
- Xcode build integration, `.app` bundle compilation, codesigning, and `.ipa` distribution archive.
- Exit criteria: Signed `.ipa` successfully deployed to TestFlight and verified on physical iOS devices.

## Phase 5 – Web (WebAssembly & WebGPU/WebGL)
- WebAssembly (WASM) build pipeline via Emscripten / Clang.
- Dual canvas renderer: WebGPU primary with automated WebGL 2.0 / WebGL 1.0 fallback.
- Browser event loop synchronization, DOM events, and touch handling.
- Web accessibility bridge (ARIA live regions and semantic DOM overlay synchronization).
- PWA service worker and manifest generation.
- Exit criteria: Web sample application renders smoothly at 60 FPS in Chrome, Safari, and Firefox.
- **MILESTONE: V1.0 STABLE PRODUCTION RELEASE FOR 6 CORE PLATFORMS.**

---

## Extended Roadmap (Form-Factor Expansion)

## Phase 6 – Android TV & Google TV
- Integration of `PingDart.TV` module for Android TV / Google TV.
- 10-foot UI layouts, overscan margin protection, and high-contrast focus rendering.
- D-Pad directional navigation and focus highlight state engine.
- Media session playback and TV launcher recommendation channels.
- Exit criteria: TV application navigates fluidly using physical TV remote control.

## Phase 7 – watchOS & Wear OS
- Integration of `PingDart.Wearable` module.
- Low-power, ultra-compact memory footprint and aggressive CPU throttling adaptations.
- Rotary input listeners (Apple Watch Digital Crown, Galaxy Watch rotating bezel).
- Apple WatchKit / SwiftUI companion integration and Wear OS Tiles API.
- HealthKit and Health Services sensor streaming.
- Exit criteria: Functional standalone/companion wearable applications on Apple Watch and Galaxy Watch.

## Phase 8 – tvOS & visionOS
- tvOS platform adapter with Siri Remote touch/clickpad focus handling.
- `PingDart.Spatial` module for visionOS: RealityKit bridge, 3D volumetric windows, and spatial audio.
- Eye-tracking and hand-gesture recognition mapping.
- Exit criteria: Spatial application running in visionOS simulator and Apple Vision Pro device labs.

## Phase 9 – Embedded Linux & QNX
- Integration of `PingDart.Embedded` and `PingDart.Automotive` modules.
- Direct-to-display rendering (Linux DRM/KMS, Wayland-IVI, QNX Screen graphics subsystem).
- Deterministic, zero-garbage-collection RTOS runtime for QNX Neutrino 7.x/8.x.
- CAN bus, automotive telemetry, and multi-display digital cluster rendering.
- Exit criteria: PingDart cluster UI running at rock-solid 60 FPS on automotive reference board.

---

# 26. Acceptance Criteria

The framework will be considered ready for its first production release when:

1. A single application source project can target all six supported platforms.
2. Core UI and business logic are shared.
3. Each supported platform has a functioning native integration layer.
4. Applications can access required platform capabilities through documented APIs.
5. The rendering engine passes its reference performance tests.
6. The compiler and build pipeline are reproducible.
7. Automated tests pass on supported release targets.
8. Hot reload works for supported development changes.
9. Package management works.
10. Documentation covers application development and deployment.
11. Security and dependency audits are completed.
12. Versioning and backward compatibility policies are published.

A platform must not be advertised as production-ready until its own acceptance tests pass.

---

# 27. Major Technical Risks

| Risk | Mitigation |
|---|---|
| Six-platform complexity | Incremental platform rollout |
| GPU backend differences | Rendering abstraction |
| iOS restrictions | Native Apple integration |
| Compiler complexity | Reuse LLVM/Clang |
| Memory errors | RAII, sanitizers, static analysis |
| UI performance | Profiling and display-list architecture |
| Browser limitations | Dedicated WebAssembly backend |
| Plugin security | Permission and trust model |
| Build complexity | Unified CLI and CMake |
| API instability | Semantic versioning and compatibility tests |

---

# 28. Open-Source and Commercial Strategy

The project should separate its technology into clear licensing and distribution components.

Potential model:

**Community Edition**
- Core framework.
- UI widgets.
- Standard platform adapters.
- CLI.
- Documentation.

**Professional Edition**
- Advanced profiling.
- Enterprise tooling.
- Extended development utilities.
- Additional support.

**Enterprise Edition**
- Commercial support.
- Private package repositories.
- Extended maintenance.
- Enterprise build infrastructure.
- Organization-level tooling.

Final licensing must be reviewed for every dependency, particularly LLVM, Skia and platform SDKs.

---

# 29. Final Recommended Technology Stack

| Component | Selection |
|---|---|
| Core engine | C++20 |
| Existing compiler integration | PingDartCore Rust, subject to evaluation |
| C++ compiler | LLVM / Clang |
| Build system | CMake + Ninja |
| UI framework | Custom PingDart UI |
| Graphics | Skia evaluation + native GPU backends |
| Android | Android NDK |
| iOS | Objective-C++ + Apple SDK |
| Windows | Win32 / C++/WinRT |
| macOS | Objective-C++ / AppKit |
| Linux | X11 / Wayland |
| Web | WebAssembly |
| Package manager | Custom `pd` |
| Developer CLI | C++ or Rust |
| Testing | Unit + Integration + Golden Tests |
| CI/CD | Cross-platform automated builds |

---

# 30. Final Product Vision

PingDart Framework should become an independent application development platform with its own:

- Programming APIs.
- Core engine.
- Compiler integration.
- Rendering engine.
- UI component library.
- Platform adapters.
- Package ecosystem.
- Developer tools.
- Documentation.
- Release infrastructure.

The framework must not be a thin wrapper around Qt, Flutter or another existing UI framework.

It may reuse established technologies such as LLVM, Clang, Skia and native platform SDKs while retaining ownership of its public APIs, runtime architecture, UI system and developer experience.

**Final engineering principle:**

Build the core once, reuse the application logic everywhere, and integrate with each operating system through a dedicated platform layer.

**Product ambition:**

PingDart Framework – One Codebase. Multiple Platforms. Native Capabilities. High Performance.

---

# 31. PingDart Platform Compatibility Matrix

The following matrix formally specifies supported OS versions, rendering modes, lifecycle architectures, native bridges, and build deliverables across all deployment targets.

| Platform Target | Phase | Min OS Version | Supported Rendering Modes | Lifecycle Model | Native Bridge / Interop | Build Deliverables |
|---|---|---|---|---|---|---|
| **Android** | Phase 2 | Android 8.0 (API 26) | Custom, Native, Hybrid | Activity / SurfaceView / Process | JNI + NDK C/C++ + Kotlin/Java bridge | `.apk`, `.aab`, `.so` |
| **iOS** | Phase 4 | iOS 15.0 | Custom, Native, Hybrid | UIApplicationDelegate / SceneDelegate | Objective-C++ / Swift Interop | `.app` bundle, `.ipa` package |
| **Windows** | Phase 1 | Windows 10 (1809+) | Custom, Native, Hybrid | Win32 Message Loop / WinRT | C++/WinRT, Win32 API | `.exe`, `.dll`, `.msix`, WiX MSI |
| **macOS** | Phase 3 | macOS 12.0 (Monterey) | Custom, Native, Hybrid | NSApplicationDelegate | Objective-C++ / AppKit | `.app` bundle, `.dmg`, `.pkg` |
| **Linux** | Phase 1 | GLIBC 2.31+ / Wayland / X11 | Custom, Native, Hybrid | POSIX Event Loop / epoll | X11 / Wayland / AT-SPI2 C API | ELF executable, AppImage, Flatpak |
| **Web** | Phase 5 | WASM + WebGL2 / WebGPU browsers | Custom (WebGPU/Canvas) | Browser event loop & requestAnimationFrame | Emscripten Embind / JS Glue | `.html`, `.js`, `.wasm`, Assets |
| **Android TV / Google TV** | Phase 6 | Android 9.0 (API 28) | Custom, Native, Hybrid | Android TV Leanback Activity | JNI + Leanback Focus Engine | TV `.apk`, TV `.aab` |
| **watchOS** | Phase 7 | watchOS 9.0 | Custom (Optimized), Native | WKExtensionDelegate / SwiftUI Host | Objective-C++ / WatchKit | watchOS `.app` bundle |
| **Wear OS** | Phase 7 | Wear OS 3.0 (API 30) | Custom (Optimized), Native | Wearable Activity / AmbientLifecycle | JNI + Wearable SDK | Wear OS `.apk`, `.aab` |
| **tvOS** | Phase 8 | tvOS 16.0 | Custom, Native, Hybrid | UIApplicationDelegate / TV Focus | Objective-C++ / tvOS SDK | tvOS `.app`, `.ipa` |
| **visionOS** | Phase 8 | visionOS 1.0 | Custom, Spatial (3D), Hybrid | RealityKit / UIWindowScene | Objective-C++ / RealityKit bridge | visionOS `.app`, `.ipa` |
| **Embedded Linux** | Phase 9 | Linux Kernel 5.4+ (Yocto) | Custom (DRM/KMS) | Single-process / systemd service | Direct DRM/KMS, evdev, CAN socket | Stripped ELF, rootfs layer |
| **QNX** | Phase 9 | QNX Neutrino 7.1+ / 8.0 | Custom (QNX Screen) | POSIX / QNX Resource Manager | QNX Screen API, PPS (Persistent Publish/Subscribe) | Cross-compiled QNX binary |

### Matrix Key & Implementation Rules:
1. **Tri-Modal Rendering Rules:**
   - *Custom Rendering:* Full custom GPU display-list rendering (Vulkan, Metal, D3D12, WebGPU).
   - *Native Rendering:* Platform views directly embedding native widgets into the layout tree.
   - *Hybrid Rendering:* Compositing custom canvas drawing with native controls and platform textures.
2. **Accessibility Parity:** Custom rendering pipelines must bind to platform accessibility adapters (Android `AccessibilityNodeInfo`, iOS `UIAccessibility`, Windows UI Automation, Linux AT-SPI2, Web ARIA).
3. **Packaging Integrity:** Every target produces standard, store-compliant distribution formats. Development targets produce unbundled or debug binaries for fast iteration.
4. **Phased Scope Policy:** Features for Phases 6–9 will not block or delay the V1.0 release milestone of the core six targets.

---

# 32. Technical Architecture Freeze & Engineering Specification Roadmap

With the completion and structural validation of this PRD, the project reaches its formal **Technical Architecture Freeze** milestone. Before writing production implementation code, eight detailed technical specifications must be drafted and approved.

## 32.1 Required Engineering Architecture Documents

The following design specifications will translate the PRD into implementation-ready engineering blueprints:

| Document | Title | Core Focus |
|---|---|---|
| **DOC-01** | **Core Runtime Architecture** | C++20 engine loop, scheduler, memory arenas, reference counting / smart pointers, and platform abstraction boundaries. |
| **DOC-02** | **Rendering Engine Architecture** | GPU abstraction layer (Vulkan, Metal, D3D12, WebGPU), Skia evaluation, display list compositor, texture caches, and software fallback. |
| **DOC-03** | **Platform Adapter SDK** | C++ PAL interfaces, Win32/X11/Wayland/AppKit window hosts, Android JNI bridge, iOS Objective-C++ bridge, and accessibility adapters. |
| **DOC-04** | **Compiler & Build System Architecture** | CMake toolchains, Ninja integration, SDK discovery, Clang cross-compilation matrix, hermetic dependency management, and codesigning pipelines. |
| **DOC-05** | **Widget & Layout Engine Specification** | Declarative widget tree lifecycle (Widget -> Element -> RenderNode), layout constraint solver (flex/grid/absolute), and animation controllers. |
| **DOC-06** | **Plugin SDK & Permission System** | Plugin ABI boundaries, `plugin.yaml` manifests, dynamic loading, permission enforcement, and signal/crash isolation. |
| **DOC-07** | **Testing & Benchmark Strategy** | Unit testing harnesses, golden pixel testing suites, HarfBuzz text shaping conformance, multi-DPI layout tests, and 60/120 FPS frame benchmarks. |
| **DOC-08** | **V1.0 Development Task Breakdown** | Granular epic, feature, and task breakdown with ticket IDs, dependency graphs, and sprint allocation across Phase 1 through Phase 5. |

## 32.2 Critical Compiler Integration Strategy (Rust ↔ C++ FFI Boundary)

To eliminate architectural risk and prevent language boundary instability:
- **Separation of Concerns:** Keep **PingDartCore** (the existing Rust compiler project) completely isolated from the new C++ runtime engine initially.
- **Strict C-ABI / FFI Contract:**
  - Communication across the boundary must use a stable, versioned C-ABI (`extern "C"`).
  - Use opaque pointer handles (e.g., `pd_compiler_ctx_t*`, `pd_ast_t*`) and explicit fixed-width primitive types (`int32_t`, `uint64_t`, `uint8_t*`). Never pass non-POD C++ objects or Rust structs directly by value.
  - **Explicit Ownership Rules:** Memory allocated by Rust must be released by Rust (`pd_free_buffer(...)`); memory allocated by C++ must be released by C++.
  - **Zero Exception / Panic Propagation:** Neither C++ exceptions nor Rust panics may ever unwind across the FFI boundary. All Rust FFI entry points must be wrapped with `std::panic::catch_unwind`, and C++ callers must catch exceptions internally, translating boundary results into explicit integer status codes (`pd_status_t`).
- **Prototype Validation:** Validate the Rust-to-C++ interaction via an independent working prototype before committing to any tightly coupled compiler-runtime integration.

## 32.3 Architecture Decision Records (ADRs) Policy

An architecture freeze locks interface contracts, subsystem boundaries, and macro-level design decisions. It does **not** prohibit incremental optimizations or design improvements discovered during hands-on prototyping. 

To maintain architectural integrity:
- Any modification, refinement, or interface evolution discovered during prototyping must be recorded in an **Architecture Decision Record (ADR)** under `doc/adr/` (e.g., `doc/adr/ADR-0001-event-loop-backend-selection.md`).
- ADRs track: Context, Decision, Consequences, Status (Proposed, Accepted, Superceded), and Compliance Notes.

## 32.4 Recommended Execution Order & Desktop Proof-of-Concept

To deliver a working engine systematically without building all nine phases simultaneously, execution follows this strict sequence:

1. **DOC-01: Core Runtime Architecture** (Event loop, task scheduler, memory ownership, object lifecycle, threading, FFI).
2. **DOC-03: Platform Adapter SDK** (Windows Win32 and Linux X11/Wayland window hosts, surface creation, input dispatch).
3. **DOC-02: Rendering Engine Architecture** (Validate one GPU rendering backend—Vulkan on Linux, D3D12 on Windows—before introducing additional APIs).
4. **DOC-05: Widget & Layout Engine** (Widget creation, layout constraints, render invalidation, event propagation).
5. **Desktop Proof-of-Concept (POC):** Run the exact same sample C++ application on Windows and Linux using shared framework code.

### Desktop POC Acceptance Criteria:
The prototype must demonstrate:
- Native application host window on Windows 11 and Ubuntu Linux.
- Functioning deterministic event loop (non-busy waiting).
- GPU-backed rendering surface displaying custom-drawn graphics.
- Simple widget tree (Scaffold, Column, Text, interactive Button).
- Pointer (mouse) and keyboard input events correctly routed.
- Fluid window resizing without flicker or tearing.
- Clean process shutdown with zero memory leaks.
- Zero platform-specific code inside the application-level `main.cpp`.
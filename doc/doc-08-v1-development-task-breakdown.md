# PingDart Framework
## DOC-08: V1.0 Development Task Breakdown Specification

**Status:** Technical Architecture Specification  
**Version:** 1.0  
**Parent Documents:** [PingDart PRD](prd.md) · [DOC-01](doc-01-core-runtime-architecture.md) · [DOC-02](doc-02-rendering-engine-architecture.md) · [DOC-03](doc-03-platform-adapter-sdk.md) · [DOC-04](doc-04-compiler-build-system.md) · [DOC-05](doc-05-widget-layout-engine.md) · [DOC-06](doc-06-plugin-sdk-security.md) · [DOC-07](doc-07-testing-benchmark-strategy.md)  
**Target Milestone:** Phase 1 V1.0 Desktop Production Release

---

# 1. Executive Summary & Delivery Milestones

This document translates the complete architecture established in **DOC-01 through DOC-07** into an actionable, dependency-ordered engineering backlog.

To ensure rapid validation and mitigate risk, Phase 1 execution is organized around **Three Concrete Milestones**:

```text
+--------------------------------------------------------------------------+
|                     MILESTONE 1 (M1): DESKTOP HOST                       |
|   - Native Windows (Win32) & Linux (Wayland & X11) window hosts          |
|   - DOC-01 Core EventLoop & cooperative event pump integration           |
|   - Normalized pointer and keyboard input dispatch                       |
|   - Dynamic window resizing and clean process termination                |
+--------------------------------------------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
|                    MILESTONE 2 (M2): INTERACTIVE UI                      |
|   - RHI Vulkan swapchain creation on Windows and Linux surfaces          |
|   - Display list recording and rasterization                             |
|   - Three-tree engine (Widget -> Element -> RenderObject)                |
|   - Interactive counter demo: Button click -> dirty subtree rebuild     |
+--------------------------------------------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
|               MILESTONE 3 (M3): V1.0 RELEASE CANDIDATE                   |
|   - Official Developer CLI (`pd`) and project scaffolding                |
|   - Native desktop packaging (Windows MSIX/portable, Linux AppImage)     |
|   - Plugin SDK, Capability Broker, and sandboxed out-of-process IPC      |
|   - Full CI matrix, automated golden tests, and benchmark verification   |
+--------------------------------------------------------------------------+
```

---

# 2. Epic Hierarchy & Dependency Map

The engineering backlog is structured into eight core epics:

```text
[PD-EPIC-001: Build Foundation] --------------------------+
       |                                                  |
       v                                                  v
[PD-EPIC-002: Core Runtime]                     [PD-EPIC-006: CLI & Packaging]
       |                                                  |
       v                                                  |
[PD-EPIC-003: Windows/Linux PAL]                          |
       |                                                  |
       v                                                  v
[PD-EPIC-004: Vulkan RHI]                       [PD-EPIC-007: Plugin Security]
       |                                                  |
       +-----------------------+                          |
                               v                          v
                        [PD-EPIC-005: Widget & Layout Engine]
                                       |
                                       v
                        [PD-EPIC-008: Integration & Release]
```

### Epic Overview Table
| Epic ID | Title | Primary Dependencies | Target Milestone |
|---|---|---|---|
| **PD-EPIC-001** | Repository Structure & Hermetic Build Foundation | None | M1 |
| **PD-EPIC-002** | Core Runtime, Scheduler & Memory Engine | PD-EPIC-001 | M1 |
| **PD-EPIC-003** | Platform Abstraction Layer (PAL) Windows & Linux | PD-EPIC-002 | M1 |
| **PD-EPIC-004** | Rendering Hardware Interface (RHI) & Vulkan | PD-EPIC-003 | M2 |
| **PD-EPIC-005** | Declarative Widget Tree & Layout Engine | PD-EPIC-002, PD-EPIC-004 | M2 |
| **PD-EPIC-006** | Official Developer CLI (`pd`) & Packaging | PD-EPIC-001, PD-EPIC-003 | M3 |
| **PD-EPIC-007** | Plugin SDK, Capability Broker & Sandboxed IPC | PD-EPIC-002, PD-EPIC-006 | M3 |
| **PD-EPIC-008** | System Integration, Verification & Release | PD-EPIC-003 through 007 | M3 |
| **PD-EPIC-009** | First-Class Web Platform, SSR & Universal SEO Engine | PD-EPIC-002, PD-EPIC-005 | M2/M3 |

---

# 3. Granular Engineering Task Backlog

---

### Epic 1: Repository Structure & Hermetic Build Foundation (PD-EPIC-001)

#### `ENG-101`: Canonical CMake Target Hierarchy
- **Owner:** Build Engineer  
- **Estimate:** 3 Days  
- **Dependencies:** None  
- **Description:** Establish root `CMakeLists.txt` and modular build files defining targets: `pingdart_core`, `pingdart_pal`, `pingdart_rhi`, `pingdart_ui`, and `pd_cli`. Configure C++20 standard, strict warnings (`-Wall -Wextra -Werror` / `/W4 /WX`), and path remapping (`-ffile-prefix-map`).
- **Acceptance Criteria:** `cmake -B build -G Ninja` configures cleanly on Windows (MSVC 2022) and Linux (Clang 16).
- **Automated Tests:** CI configure check across both platforms.

#### `ENG-102`: Hermetic Lockfile & FetchContent Management
- **Owner:** Build Engineer  
- **Estimate:** 3 Days  
- **Dependencies:** ENG-101  
- **Description:** Implement `pingdart.lock` parser and CMake dependency manager downloading Skia, HarfBuzz, FreeType, SQLite, and GoogleTest with cryptographic SHA-256 validation.
- **Acceptance Criteria:** Build aborts with `SecurityViolation` if any archive hash mismatches. Clean build works with zero unpinned host packages.
- **Automated Tests:** Unit test verifying hash validation failure on tampered archive.

---

### Epic 2: Core Runtime Engine (PD-EPIC-002)

#### `ENG-201`: Value-Based Status & Result Pattern
- **Owner:** Core Runtime Engineer  
- **Estimate:** 2 Days  
- **Dependencies:** ENG-101  
- **Description:** Implement `pingdart::core::Status` and `pingdart::core::Result<T>` ([DOC-01 § 7.1](doc-01-core-runtime-architecture.md#L173-L255)).
- **Acceptance Criteria:** Move-only types supported; zero dynamic heap allocations on `Ok` path; `has_value()`, `value()`, and `status()` pass unit tests.
- **Automated Tests:** GoogleTest suite `test_result.cpp` with 100% statement coverage.

#### `ENG-202`: Intrusive Reference Counting (`RefPtr<T>`)
- **Owner:** Core Runtime Engineer  
- **Estimate:** 3 Days  
- **Dependencies:** ENG-201  
- **Description:** Implement `RefCounted<T>` and `RefPtr<T>` ([DOC-01 § 7.2](doc-01-core-runtime-architecture.md#L257-L345)). Enforce atomic `memory_order_relaxed` on `add_ref` and `memory_order_acq_rel` on `release`.
- **Acceptance Criteria:** Zero secondary control-block heap allocations; safe copy/move assignment; verified by AddressSanitizer.
- **Automated Tests:** GoogleTest suite `test_ref_ptr.cpp` (concurrency stress test with 8 worker threads).

#### `ENG-203`: Master EventLoop & Priority Task Scheduler
- **Owner:** Core Runtime Engineer  
- **Estimate:** 5 Days  
- **Dependencies:** ENG-202  
- **Description:** Implement `EventLoop` ([DOC-01 § 7.3](doc-01-core-runtime-architecture.md#L347-L395)). Support `post_task`, `post_delayed_task`, `post_microtask`, and non-busy sleep via condition variables and platform wakeup handles.
- **Technical Risk:** Thread contention and race conditions when posting from worker pools.
- **Prototype Evidence:** Verified compilable prototype from DOC-01.
- **Acceptance Criteria:** Tasks execute in priority order; microtasks flush before next macrotask; clean shutdown via `terminate(exit_code)`.
- **Automated Tests:** ThreadSanitizer suite `test_event_loop.cpp` verifying zero data races.

#### `ENG-204`: PingDartCore Rust FFI Boundary Contract
- **Owner:** Systems Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-201  
- **Description:** Implement `pd_compiler_abi.h` C-ABI boundary in Rust (`PingDartCore`) and C++ wrapper. Enforce `std::panic::catch_unwind` and explicit allocation/free symmetry (`pd_compiler_free_buffer`).
- **Acceptance Criteria:** Version handshake passes; Rust memory freed by Rust; intentional panic captured as error code without crashing host.
- **Automated Tests:** Cargo test + CTest integration harness `test_compiler_ffi.cpp`.

---

### Epic 3: Platform Abstraction Layer (PAL) Windows & Linux (PD-EPIC-003)

#### `ENG-301`: PAL Interface Contracts & Native Surface Tokens
- **Owner:** Platform Engineer  
- **Estimate:** 2 Days  
- **Dependencies:** ENG-201  
- **Description:** Implement header definitions for `IPlatformHost`, `IWindow`, `ISurfaceProvider`, `IInputDispatcher`, `IPlatformEventPump`, and `ICapabilityProvider` ([DOC-03 § 3](doc-03-platform-adapter-sdk.md#L90-L450)).
- **Acceptance Criteria:** Compiles cleanly under C++20 with zero warnings.

#### `ENG-302`: Win32 Native Host Window & Event Pump
- **Owner:** Windows Systems Engineer  
- **Estimate:** 5 Days  
- **Dependencies:** ENG-301, ENG-203  
- **Description:** Implement `Win32Window` and `Win32EventPump`. Configure per-monitor v2 DPI awareness (`SetProcessDpiAwarenessContext`), window class registration, `MsgWaitForMultipleObjectsEx` integration respecting next timer deadline, and `WM_SIZE`/`WM_CLOSE` dispatch.
- **Acceptance Criteria:** Window displays on Windows 11; resizes without flickering (`WM_ERASEBKGND` handled); exits cleanly.
- **Automated Tests:** CTest suite `test_win32_window.cpp` (opens, resizes, and closes headless/visible window).

#### `ENG-303`: Linux Wayland Backend with Multi-Thread Event Loop
- **Owner:** Linux Systems Engineer  
- **Estimate:** 6 Days  
- **Dependencies:** ENG-301, ENG-203  
- **Description:** Implement `WaylandWindow` and `WaylandEventPump`. Bind `wl_compositor`, `xdg_wm_base`, `wl_seat`, `wp_fractional_scale_manager_v1`. Implement canonical prepare-read loop ([DOC-03 § 2.2](doc-03-platform-adapter-sdk.md#L67-L82)) with `epoll` and `wl_display_cancel_read` on wakeups.
- **Technical Risk:** Wayland client deadlocks if `wl_display_prepare_read` fails to handle pending queue.
- **Acceptance Criteria:** Native Wayland surface created; fractional scaling handled; zero socket deadlocks.
- **Automated Tests:** Wayland mock compositor test harness running in headless Weston container.

#### `ENG-304`: Linux X11 Fallback Backend & Startup Resolver
- **Owner:** Linux Systems Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-303  
- **Description:** Implement `X11Window` and startup detector resolving `WAYLAND_DISPLAY` $\to$ `DISPLAY`. Handle `WM_DELETE_WINDOW` protocol and register connection socket with `epoll`.
- **Acceptance Criteria:** Correctly selects Wayland when present; falls back to X11 when `WAYLAND_DISPLAY` is absent or `PINGDART_BACKEND=x11` is set.
- **Automated Tests:** Dual-backend test suite running under both Xvfb and Weston.

#### `ENG-305`: Raw Input Normalization & XKB Keyboard Translation
- **Owner:** Platform Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-302, ENG-303  
- **Description:** Translate Win32 messages and Wayland/X11 inputs into normalized `PointerEvent` and `KeyboardEvent` ([DOC-03 § 3.2](doc-03-platform-adapter-sdk.md#L150-L240)). Integrate `libxkbcommon` on Linux for dead-key and modifier mapping.
- **Acceptance Criteria:** Mouse click coordinates and keyboard strokes match physical events within sub-pixel precision across platforms.
- **Automated Tests:** Synthetic input injection unit test suite.

---

### Epic 4: Rendering Hardware Interface (RHI) & Vulkan Pipeline (PD-EPIC-004)

#### `ENG-401`: RHI Abstract Hardware Interface
- **Owner:** Graphics Engineer  
- **Estimate:** 3 Days  
- **Dependencies:** ENG-301  
- **Description:** Implement `IRenderDevice`, `ICommandBuffer`, `ITexture`, `ISwapchain`, and `IPipelineState` ([DOC-02 § 2](doc-02-rendering-engine-architecture.md#L47-L188)).
- **Acceptance Criteria:** Full interface abstraction completed; returns value-based `Result<T>`.

#### `ENG-402`: Vulkan WSI Surface & Swapchain Implementation
- **Owner:** Vulkan Graphics Engineer  
- **Estimate:** 6 Days  
- **Dependencies:** ENG-401, ENG-302, ENG-303  
- **Description:** Implement Vulkan 1.2+ backend. Support `VK_KHR_win32_surface`, `VK_KHR_wayland_surface`, `VK_KHR_xcb_surface`. Allocate triple-buffered swapchains with `VkSemaphore` synchronization and `VK_ERROR_OUT_OF_DATE_KHR` dynamic recreation ([DOC-02 § 3](doc-02-rendering-engine-architecture.md#L190-L226)).
- **Technical Risk:** Driver-specific swapchain recreation crashes on window minimization/resizing.
- **Prototype Evidence:** Working clear-color swapchain prototype.
- **Acceptance Criteria:** Swapchain resizes dynamically without crash; zero Vulkan validation layer warnings.
- **Automated Tests:** Vulkan validation layer automated test running clear-color frame presentations.

#### `ENG-403`: DisplayList Builder & 2D Compositor Pipeline
- **Owner:** Graphics Engineer  
- **Estimate:** 5 Days  
- **Dependencies:** ENG-401  
- **Description:** Implement `DisplayListBuilder` and `DisplayList` ([DOC-02 § 5](doc-02-rendering-engine-architecture.md#L262-L295)). Support immutable recording of transforms, clips, rounded rectangles, text blobs, and images, along with damage bounding box tracking (`DamageRegion`).
- **Acceptance Criteria:** Replays paint commands deterministically; skips undamaged regions during partial redraws.
- **Automated Tests:** Unit tests verifying display list serialization, clipping operations, and dirty rect unions.

#### `ENG-404`: Direct3D 12 Backend for Windows 10/11
- **Owner:** Windows Graphics Engineer  
- **Estimate:** 6 Days  
- **Dependencies:** ENG-401, ENG-302  
- **Description:** Implement D3D12 backend using `IDXGIFactory7`, `IDXGIAdapter4`, direct command queues, and `DXGI_SWAP_EFFECT_FLIP_DISCARD` flip-model swapchain with tearing support ([DOC-02 § 4](doc-02-rendering-engine-architecture.md#L228-L260)).
- **Acceptance Criteria:** D3D12 device renders and presents cleanly on Windows 11 with zero debug layer errors.
- **Automated Tests:** D3D12 automated presentation test on Windows CI runner.

#### `ENG-405`: GPU Device-Loss Detection & Resilient Recovery
- **Owner:** Graphics Systems Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-401, ENG-402, ENG-404  
- **Description:** Implement resilient GPU device removal and device-loss recovery loop (`VK_ERROR_DEVICE_LOST` / `DXGI_ERROR_DEVICE_REMOVED` / `DXGI_ERROR_DEVICE_RESET`). Tear down pipeline state caches, release stale swapchains, poll or wait for device restoration, re-enumerate adapters, recreate device and swapchain, and trigger tree redraw without process crash ([DOC-02 § 6](doc-02-rendering-engine-architecture.md#L300-L340)).
- **Acceptance Criteria:** Simulated device loss triggers full teardown and reconstruction; render loop resumes without crash or resource leak.
- **Automated Tests:** Automated unit and mock tests asserting clean device recreation on simulated `DeviceLost` return code.

---

### Epic 5: Declarative Widget Tree & Layout Engine (PD-EPIC-005)

#### `ENG-501`: Widget, Element, and RenderObject Base Hierarchy
- **Owner:** UI Framework Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-202  
- **Description:** Implement base classes for `Widget`, `StatelessWidget`, `StatefulWidget`, `Element`, `RenderObject`, and `BuildContext` ([DOC-05 § 10](doc-05-widget-layout-engine.md#L241-L417)).
- **Acceptance Criteria:** Inflates element tree from widget tree; maintains strict parent-owns-child hierarchy.
- **Automated Tests:** Unit test verifying element mount, update, and unmount lifecycles.

#### `ENG-502`: Multi-Child Sibling Reconciliation & Key Scoping
- **Owner:** UI Framework Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-501  
- **Description:** Implement reconciliation diffing algorithm ([DOC-05 § 3.2](doc-05-widget-layout-engine.md#L88-L103)). Enforce sibling key scoping, duplicate key assertions in debug mode, and efficient element reuse when keyed children reorder.
- **Acceptance Criteria:** Reordering children updates layout without recreating underlying elements or dropping state.
- **Automated Tests:** Comprehensive reconciliation test suite verifying keyed child permutations.

#### `ENG-503`: One-Pass BoxConstraints Solver & Layout Primitives
- **Owner:** UI Framework Engineer  
- **Estimate:** 6 Days  
- **Dependencies:** ENG-501  
- **Description:** Implement `BoxConstraints` and layout primitives (`Row`, `Column`, `Flex`, `Expanded`, `Stack`, `Padding`, `Center`, `SizedBox`) ([DOC-05 § 4 & 5](doc-05-widget-layout-engine.md#L143-L182)). Enforce NaN rejection and flex-inside-unbounded assertions.
- **Acceptance Criteria:** Children correctly size and position; unbounded flex violations assert cleanly; overflow renders visual hazard strips in Debug mode.
- **Automated Tests:** Constraint solver test suite covering tight, loose, unbounded, and overflow states.

#### `ENG-504`: Reactive State Management & Batched Microtask Rebuilds
- **Owner:** UI Framework Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-501, ENG-203  
- **Description:** Implement `State<T>` and `set_state()`. Ensure `set_state` asserts UI-thread execution, disallows calls on defunct states, and batches dirty element rebuilds via DOC-01 `post_microtask` sorted by tree depth ([DOC-05 § 6](doc-05-widget-layout-engine.md#L184-L208)).
- **Acceptance Criteria:** Multiple `set_state()` calls within a single turn batch into a single rebuild pass before frame paint.
- **Automated Tests:** State batching test verifying exactly one rebuild occurs for multiple synchronous state mutations.

#### `ENG-505`: Text Shaping & HarfBuzz Typography Integration
- **Owner:** UI / Typography Engineer  
- **Estimate:** 5 Days  
- **Dependencies:** ENG-503, ENG-403  
- **Description:** Integrate HarfBuzz and FreeType to shape Unicode strings, resolve BiDi text runs, compute line wraps, and output shaped glyph blobs into `DisplayList` ([DOC-05 § 8](doc-05-widget-layout-engine.md#L249-L270)).
- **Acceptance Criteria:** Correctly renders complex scripts (Arabic, Devanagari, English ligatures) with accurate line wrapping.
- **Automated Tests:** Typography golden image test suite verifying glyph positions.

---

### Epic 6: Official Developer CLI (`pd`) & Desktop Packaging (PD-EPIC-006)

#### `ENG-601`: Developer CLI Core (`pd doctor`, `pd init`, `pd build`, `pd run`)
- **Owner:** Tooling Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-101  
- **Description:** Author `pd` CLI executable in C++ or Rust. Implement `pd doctor` verifying Clang/MSVC, Ninja, CMake, and Vulkan SDK. Implement project scaffolding and build orchestration ([DOC-04 § 5](doc-04-compiler-build-system.md#L190-L232)).
- **Acceptance Criteria:** `pd doctor` reports accurate diagnostic state; `pd init my_app` creates compilable scaffold.
- **Automated Tests:** CLI functional test suite executing commands in subshells.

#### `ENG-602`: Windows Self-Contained & MSIX Packaging Pipeline
- **Owner:** Windows Tooling Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-601, ENG-302  
- **Description:** Implement `pd package windows`. Bundle required VC++ redistributable DLLs, D3D12 Agility SDK, Vulkan loader, and third-party license notices. Generate `AppxManifest.xml` and package signed `.msix` installers via `MakeAppx` and `SignTool` ([DOC-04 § 6.1](doc-04-compiler-build-system.md#L248-L260)).
- **Acceptance Criteria:** Packaged portable folder and MSIX install and run cleanly on a fresh vanilla Windows 11 VM.
- **Automated Tests:** Clean VM installation smoke test.

#### `ENG-603`: Linux AppImage & Flatpak Packaging Pipeline
- **Owner:** Linux Tooling Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-601, ENG-303  
- **Description:** Implement `pd package linux`. Assemble AppDir with `AppRun`, desktop file, icons, and bundled `libxkbcommon`. Execute `appimagetool` to output standalone `.AppImage` ([DOC-04 § 6.2](doc-04-compiler-build-system.md#L262-L280)).
- **Acceptance Criteria:** Generated `.AppImage` runs out-of-the-box on Ubuntu 20.04, Ubuntu 22.04, and Fedora without extra dependency installs.
- **Automated Tests:** AppImage execution smoke test on clean container images.

---

### Epic 7: Plugin SDK, Capability Broker & Sandboxing (PD-EPIC-007)

#### `ENG-701`: Plugin Manifest Parser & Stable C-ABI Loader
- **Owner:** Systems Security Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-201  
- **Description:** Implement `plugin.yaml` schema validator and dynamic library loader binding to `pd_plugin_abi.h` ([DOC-06 § 2 & 3](doc-06-plugin-sdk-security.md#L50-L148)). Verify ABI versions and negotiate capability manifests.
- **Acceptance Criteria:** Sample dynamic plugin loads, queries ABI, and registers with host.
- **Automated Tests:** GoogleTest suite verifying manifest parsing and version mismatch rejections.

#### `ENG-702`: Capability Broker & Permission Enforcement
- **Owner:** Systems Security Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-701  
- **Description:** Implement the host `CapabilityBroker`. Validate incoming capability requests against declared permissions, rate limits, and write structured security audit logs ([DOC-06 § 5](doc-06-plugin-sdk-security.md#L175-L208)).
- **Acceptance Criteria:** Authorized permissions succeed; unauthorized requests fail immediately with `PD_PLUGIN_ERR_PERMISSION_DENIED`.
- **Automated Tests:** Security unit test suite attempting authorized and unauthorized API calls.

#### `ENG-703`: Authenticated IPC & Shared Memory Transport
- **Owner:** Systems Security Engineer  
- **Estimate:** 5 Days  
- **Dependencies:** ENG-702  
- **Description:** Implement Unix domain socket (Linux) and Named Pipe (Windows) IPC. Enforce peer PID/UID verification (`SO_PEERCRED` / `GetNamedPipeClientProcessId`) and HMAC-SHA256 message signatures. Implement safe shared memory bounds checking ([DOC-06 § 7](doc-06-plugin-sdk-security.md#L244-L290)).
- **Acceptance Criteria:** Malicious or unauthenticated connections rejected; shared memory buffer reads protected against integer overflows.
- **Automated Tests:** IPC security test suite verifying HMAC validation and bounds check rejections.

#### `ENG-704`: OS Sandboxing (Windows Job/Token & Linux Seccomp/Namespaces)
- **Owner:** Systems Security Engineer  
- **Estimate:** 6 Days  
- **Dependencies:** ENG-703  
- **Description:** Implement child process sandboxing ([DOC-06 § 6](doc-06-plugin-sdk-security.md#L210-L242)). On Windows: Restricted Token (Low Integrity) + Job Object memory limits + `SetErrorMode`. On Linux: PID/Net/Mount namespaces + Seccomp-BPF syscall filters. Enforce strict **fail-closed** policy.
- **Acceptance Criteria:** Plugin cannot access unauthorized filesystem paths or spawn processes; sandbox fails closed if unprivileged namespaces are unsupported.
- **Automated Tests:** Adversarial sandbox breakout test verifying syscall blocks.

---

### Epic 8: System Integration, Verification & V1.0 Release (PD-EPIC-008)

#### `ENG-801`: Interactive Counter Desktop Demonstration Application
- **Owner:** Lead Application Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-302, ENG-303, ENG-402, ENG-504  
- **Description:** Build the reference desktop milestone application ([DOC-05 § 11](doc-05-widget-layout-engine.md#L419-L457)): Scaffold containing vertical Column, styled text headers, interactive Button, and reactive Counter.
- **Acceptance Criteria:** Identical application source builds and runs natively on Windows 11 and Ubuntu 22.04; clicking button updates only counter text subtree; zero rendering tears or memory leaks.
- **Automated Tests:** Interactive E2E automation test script verifying button click and state incrementation.

#### `ENG-802`: Visual Golden Image Test Suite (SSIM)
- **Owner:** QA / Graphics Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-402, ENG-503  
- **Description:** Implement automated screenshot comparison harness across Vulkan and D3D12 backends with configurable SSIM thresholds ($\ge 0.998$ for primitives, $\ge 0.990$ for shaped text) ([DOC-07 § 4](doc-07-testing-benchmark-strategy.md#L75-L105)).
- **Acceptance Criteria:** Pixel-diff artifacts generated automatically on golden test failures; passes on reference test suite.
- **Automated Tests:** CTest suite comparing 25 baseline widget configurations against golden masters.

#### `ENG-803`: Automated Bare-Metal Benchmark Harness & 24-Hour Soak
- **Owner:** Performance Engineer  
- **Estimate:** 5 Days  
- **Dependencies:** ENG-801  
- **Description:** Implement performance telemetry harness ([DOC-07 § 6](doc-07-testing-benchmark-strategy.md#L125-L165)). Measure frame processing time, presentation intervals, cold startup, and 24-hour continuous animation memory soak. Implement $N=30$ statistical noise regression gate.
- **Acceptance Criteria:** Meets 60Hz/120Hz budgets; 24-hour soak finishes with zero memory creep ($\Delta\text{Heap} \le 1\%$).
- **Automated Tests:** Automated benchmark suite logging telemetry JSON with statistical confidence intervals.

#### `ENG-804`: Phase 1 V1.0 Release Sign-off & Audit Closure
- **Owner:** Chief Architect & Release Lead  
- **Estimate:** 3 Days  
- **Dependencies:** All previous tickets (ENG-101 through ENG-803)  
- **Description:** Conduct final formal release audit across Gate 1 (PR), Gate 2 (Nightly), and Gate 3 (RC). Verify license compliance notices, codesigning signatures, and package installations.
- **Acceptance Criteria:** 100% of test suites pass; zero compiler warnings; signed release packages published.
- **Definition of Done:** Signed V1.0 Git tag pushed with official release binaries for Windows and Linux.

---

### Epic 9: First-Class Web Platform, SSR & Universal SEO Engine (PD-EPIC-009)

#### `WEB-101`: C++ WebAssembly Build Target & Toolchain Pipeline
- **Owner:** Web Platform Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** ENG-101, ENG-202, ENG-203  
- **Description:** Configure Emscripten toolchain and CMake WebAssembly target (`pingdart_web_wasm`). Provide event loop bridge between JavaScript browser microtask/animation frame queue and `pingdart::core::EventLoop`.
- **Acceptance Criteria:** Compiles cleanly to `.wasm` + `.js` glue; initializes inside browser and headless Node.js environments.
- **Automated Tests:** Headless Node/V8 runner asserting event loop initialization and microtask drainage.

#### `WEB-102`: Semantic HTML5 & CSS3 Renderer
- **Owner:** Web UI / Typography Engineer  
- **Estimate:** 5 Days  
- **Dependencies:** ENG-201, ENG-501  
- **Description:** Implement `HtmlSerializer` and semantic DOM element decomposition ([DOC-09 § 3](doc-09-web-seo-architecture.md#L45-L95)). Emit real semantic HTML tags (`<header>`, `<main>`, `<article>`, `<h1>`-`<h6>`, `<p>`, `<a>`, `<button>`, `<input>`) and scoped CSS flexbox/grid layout classes.
- **Acceptance Criteria:** Emits valid, compliant HTML5 and scoped CSS without runtime style injection lag.
- **Automated Tests:** Golden HTML snapshot tests verifying semantic tree generation for core widget hierarchy.

#### `WEB-103`: Browser DOM Adapter & Event Dispatcher
- **Owner:** Web Platform Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** WEB-101, ENG-305  
- **Description:** Implement `BrowserDomAdapter` wrapping direct WASM-to-DOM bindings. Map browser pointer, keyboard, focus, and input events to normalized `pingdart::pal::InputEvent` instances.
- **Acceptance Criteria:** User clicks, keypresses, and touch inputs on native DOM elements dispatch seamlessly to PingDart event handlers.
- **Automated Tests:** Headless browser unit tests verifying click, input, and keyboard event dispatch.

#### `WEB-104`: Headless Server-Side Rendering (SSR) Engine
- **Owner:** Backend Systems Engineer  
- **Estimate:** 5 Days  
- **Dependencies:** WEB-102, ENG-504  
- **Description:** Implement native C++ headless SSR engine running as an ultra-fast HTTP service ([DOC-09 § 5.2](doc-09-web-seo-architecture.md#L140-L175)). Inflates widget trees, executes initial asynchronous state resolution, injects serialized state JSON, and streams complete semantic HTML documents (TTFB < 50ms).
- **Acceptance Criteria:** Renders full HTML pages server-side without requiring a headless browser or Node.js runtime.
- **Automated Tests:** Integration test verifying SSR HTTP response containing fully populated HTML body and state payload.

#### `WEB-105`: Static Site Generation (SSG) & Prerendering Pipeline
- **Owner:** Build & Tooling Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** WEB-104, WEB-107  
- **Description:** Implement build-time static site generator discovering routes and rendering pre-computed `.html` files ([DOC-09 § 5.1](doc-09-web-seo-architecture.md#L125-L138)). Automatically generates `sitemap.xml` with last modified dates and customizable `robots.txt`.
- **Acceptance Criteria:** `pd build --platform web --mode static` writes clean static directory structure with HTML, assets, sitemap, and robots.
- **Automated Tests:** Automated build test verifying output directory file hierarchy, valid XML sitemap, and robots directives.

#### `WEB-106`: Seamless Hydration & Client-Side Navigation
- **Owner:** Web Framework Engineer  
- **Estimate:** 5 Days  
- **Dependencies:** WEB-101, WEB-103, WEB-104  
- **Description:** Implement progressive DOM hydration attaching event listeners using deterministic `data-pd-id` node keys without destroying or flashing pre-rendered HTML ([DOC-09 § 6](doc-09-web-seo-architecture.md#L178-L198)). Implement client-side router leveraging the HTML5 History API (`pushState`/`popstate`).
- **Acceptance Criteria:** Hydration completes with zero visual flashes or DOM recreation; client-side navigation updates view dynamically.
- **Automated Tests:** End-to-end browser test verifying hydration attaches listeners and link clicks update routes without page reloads.

#### `WEB-107`: Universal Routing, OpenGraph & SEO Metadata Engine
- **Owner:** Full-Stack SEO Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** WEB-102  
- **Description:** Implement `SeoMetadata` engine supporting title, description, canonical link, OpenGraph tags, Twitter cards, and Schema.org JSON-LD structured data ([DOC-09 § 4](doc-09-web-seo-architecture.md#L98-L122)). Support dynamic parameter routing (`/doctors/{slug}`) and internationalized `hreflang` tags.
- **Acceptance Criteria:** Emits compliant `<head>` metadata, OpenGraph tags, and valid JSON-LD schemas verified by Google Rich Results criteria.
- **Automated Tests:** Unit tests verifying metadata generation, schema validation, and URL parameter extraction.

#### `WEB-108`: Selective WebGPU & Canvas Island Integration
- **Owner:** Graphics & WebGPU Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** WEB-101, ENG-401  
- **Description:** Implement embedded `<canvas>` islands for high-performance graphics (charts, visualizers, 3D) embedded within semantic HTML pages, bound via WebGPU or WebGL2 while maintaining parent page accessibility and SEO.
- **Acceptance Criteria:** WebGPU canvas island renders hardware-accelerated graphics inside semantic DOM structure with proper ARIA labels.
- **Automated Tests:** Integration test verifying canvas island lifecycle, resize, and context binding.

#### `WEB-109`: Web Development Server & Asset Bundler
- **Owner:** CLI & Tooling Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** WEB-101, WEB-105, ENG-601  
- **Description:** Implement `pd dev --platform web` development server with WebSocket-based live-reloading, asset hashing, gzip/brotli compression, and production web packaging.
- **Acceptance Criteria:** Hot reload pushes updates within 100ms; production builds generate hashed assets with Brotli compression.
- **Automated Tests:** CLI integration tests verifying dev server startup, asset delivery, and static bundle emission.

#### `WEB-110`: SEO, Accessibility (a11y) & Cross-Browser Verification
- **Owner:** QA & Accessibility Engineer  
- **Estimate:** 4 Days  
- **Dependencies:** WEB-102 through WEB-107  
- **Description:** Implement automated web test harness testing Lighthouse SEO score ($\ge 98$), WCAG 2.1 AA accessibility (keyboard focus, screen reader ARIA roles), and cross-browser rendering across Chromium, Gecko, and WebKit.
- **Acceptance Criteria:** Passes Lighthouse audits ($\ge 98$ SEO, $\ge 95$ Accessibility); zero crawl errors on simulated Googlebot crawl.
- **Automated Tests:** Automated headless test suite running axe-core accessibility checks and crawl verification.

---

# 4. Critical Path & Schedule Analysis

```text
CRITICAL PATH TIMELINE:
ENG-101 (CMake) ----------------> [3 Days]
       |
ENG-201 -> ENG-202 -> ENG-203 ---> [10 Days] (Core Runtime & Scheduler)
       |
       +---> ENG-301 -> ENG-302 / ENG-303 -> [12 Days] (Desktop Host Windowing) [MILESTONE 1]
       |
       +---> WEB-101 -> WEB-102 -> WEB-104 -> [14 Days] (WebAssembly & Semantic SSR Engine)
       |
ENG-401 -> ENG-402 (Vulkan WSI) -> [9 Days]  (GPU Swapchain & Present)
       |
ENG-501 -> ENG-503 -> ENG-504 ---> [14 Days] (Layout Engine & State)        [MILESTONE 2]
       |
       +---> WEB-106 -> WEB-107 ----------> [9 Days]  (Hydration, Router & SEO Metadata Engine)
       |
ENG-601 -> ENG-602 / ENG-603 ----> [8 Days]  (CLI & Desktop Packaging)
       |
ENG-801 -> ENG-803 -> ENG-804 ---> [12 Days] (E2E Demo, Soak & RC Audit)   [MILESTONE 3]
-----------------------------------------------------------------------------------------
TOTAL CRITICAL PATH ESTIMATE: 68 Engineering Days (Parallel Web Tracks do not lengthen Critical Path)
TOTAL BACKLOG SCOPE: 42 Granular Engineering Tickets across 9 Epics (ENG-101 through WEB-110)
```

---

# 5. Definition of Done (DoD) Contract

A ticket is considered **Done** if and only if all of the following conditions are met:
1. **Code Quality:** Written in compliant C++20 (or Rust for compiler subsystems). Passes Clang-Tidy and Clang-Format with zero warnings.
2. **Compilation Cleanliness:** Compiles with zero warnings under `-Wall -Wextra -Werror` (Clang) and `/W4 /WX` (MSVC).
3. **Automated Test Coverage:** Accompanied by unit tests (GoogleTest / Cargo) achieving $\ge 90\%$ branch coverage on new logic.
4. **Sanitizer Cleanliness:** Clean execution under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).
5. **No Regressions:** Passes existing integration and visual regression suites.
6. **Documentation & ADRs:** Public APIs documented with Doxygen comments; any interface deviation recorded in `doc/adr/`.

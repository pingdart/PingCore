# PingDart Framework
## DOC-07: Testing & Benchmark Strategy Specification

**Status:** Technical Architecture Specification  
**Version:** 1.0  
**Language Baseline:** C++20 · GoogleTest · CTest · Cargo  
**Parent Documents:** [PingDart PRD](prd.md) · [DOC-01](doc-01-core-runtime-architecture.md) · [DOC-03](doc-03-platform-adapter-sdk.md) · [DOC-02](doc-02-rendering-engine-architecture.md) · [DOC-05](doc-05-widget-layout-engine.md) · [DOC-04](doc-04-compiler-build-system.md) · [DOC-06](doc-06-plugin-sdk-security.md)  
**Target Phase:** Phase 1 (Desktop Proof-of-Concept)

---

# 1. Executive Summary

The **Testing & Benchmark Strategy** establishes the empirical quality framework for PingDart. Its core objective is to prove that the individual component contracts defined across **DOC-01 through DOC-06** function seamlessly and safely when composed together.

PingDart implements a multi-layered quality assurance pyramid:

```text
+--------------------------------------------------------------------------+
|                       RELEASE ACCEPTANCE GATES                           |
|       Zero Critical Flaws · Memory Soak Tests · Benchmark Baselines      |
+--------------------------------------------------------------------------+
                                     ^
                                     |
+--------------------------------------------------------------------------+
|                     SYSTEM & PACKAGING ACCEPTANCE                        |
|       Clean VM AppImage/MSIX Deployments · 24-Hour Soak Tests            |
+--------------------------------------------------------------------------+
                                     ^
                                     |
+--------------------------------------------------------------------------+
|                  INTEGRATION, GOLDEN & SECURITY TESTS                    |
|   Cross-Module CTest · Pixel Golden Tests (SSIM) · IPC Fuzzing / AFL     |
+--------------------------------------------------------------------------+
                                     ^
                                     |
+--------------------------------------------------------------------------+
|                     SANITIZER & GPU VALIDATION                           |
|        ASan · UBSan · TSan · Vulkan Validation · D3D12 Debug Layer       |
+--------------------------------------------------------------------------+
                                     ^
                                     |
+--------------------------------------------------------------------------+
|                       UNIT & COMPONENT TESTS                             |
|          GoogleTest (C++ Runtime & UI) · Cargo Test (Rust FFI)           |
+--------------------------------------------------------------------------+
```

---

# 2. Comprehensive Test Matrix

PingDart mandates dedicated testing harnesses for each functional domain:

| Test Domain | Target Subsystems | Framework / Harness | Key Verification & Pass Criteria |
|---|---|---|---|
| **C++ Unit Tests** | Core runtime, collections, ref counting, task scheduling | **GoogleTest** (v1.14+) | 100% statement coverage on core primitives (`RefPtr`, `Result`, `EventLoop`). Zero uncaught exceptions. |
| **Rust Unit & FFI Tests** | PingDartCore compiler, AST parsing, C-ABI bindings | **Cargo Test** | Validates AST lowering, panic containment (`catch_unwind`), buffer allocation/free symmetry. |
| **Integration Tests** | PAL $\leftrightarrow$ Runtime, RHI $\leftrightarrow$ PAL, UI $\leftrightarrow$ RHI | **CTest** / Custom Harness | Validates end-to-end event propagation: window event $\to$ input dispatcher $\to$ widget hit test $\to$ state update $\to$ display list. |
| **Memory Safety** | Engine libraries, allocators, plugin memory bridges | **Clang ASan & UBSan** | Zero heap buffer overflows, use-after-free, double-frees, or undefined pointer arithmetic. |
| **Concurrency & Threading** | Event loop, worker pool, SPSC/MPSC queues | **Clang TSan** | Zero data races detected during multi-threaded stress tests (e.g. 10,000 tasks dispatched concurrently). |
| **GPU Validation** | Vulkan backend, Direct3D 12 backend, swapchains | **Vulkan Validation Layers / D3D12 Debug Layer** | Zero validation warnings or errors (`VK_LAYER_KHRONOS_validation`). Zero descriptor leaks or invalid resource barriers. |
| **UI Golden Image Tests** | Display list rasterization, text shaping, layout reflow | **Pixel Golden Comparison** | Perceptual visual equivalence across GPU backends evaluated against reference baselines using configurable SSIM. |
| **Accessibility Conformance** | Semantic tree generation, screen reader synchronization | **Platform Test Adapters** | UI Automation inspect (Windows) and AT-SPI2 bus monitors (Linux) verify semantic hierarchy, roles, and labels. |
| **Security & Fuzzing** | IPC framing protocol, plugin capability broker, sandboxes | **libFuzzer / AFL++** | Fuzzing IPC message headers and payloads; verifies sandbox escapes and unauthorized permission queries strictly fail closed. |
| **Clean Packaging Tests** | Portable EXE, MSIX, AppImage, Flatpak | **Headless VM / Container Harness** | Installs package on a clean, vanilla OS image without pre-installed developer tools; verifies execution and clean exit. |

---

# 3. Memory Safety & Concurrency Sanitation

All automated CI builds run dedicated sanitizer configurations:

### 3.1 AddressSanitizer (ASan) & UndefinedBehaviorSanitizer (UBSan)
```bash
cmake -B build-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
ninja -C build-asan
ctest --test-dir build-asan --output-on-failure
```
* **Enforcement:** Any memory leak, heap out-of-bounds read/write, misaligned pointer access, or signed integer overflow fails the build immediately with exit code 1.

### 3.2 ThreadSanitizer (TSan)
```bash
cmake -B build-tsan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer"
ninja -C build-tsan
ctest --test-dir build-tsan --output-on-failure
```
* **Enforcement:** Validates that cross-thread task posting (`EventLoop::post_task`), atomic reference counts (`RefPtr`), and worker pool synchronization contain zero data races.

---

# 4. GPU Validation & Visual Golden Testing

### 4.1 Vulkan & D3D12 Debug Validation Policy
- **Vulkan:** The `VK_LAYER_KHRONOS_validation` layer is enabled for all graphics test runs. A custom `vkCreateDebugUtilsMessengerEXT` callback converts any Vulkan warning or error into a test assertion failure.
- **Direct3D 12:** `ID3D12Debug::EnableDebugLayer()` is activated; `ID3D12InfoQueue::SetBreakOnSeverity` is configured for `CORRUPTION` and `ERROR`.

### 4.2 Configurable Visual Golden Testing (SSIM)
- The test harness renders a predetermined test widget tree offscreen to an `ITexture` render target.
- The resulting pixel buffer is saved and compared against an approved golden PNG image using Structural Similarity Index (SSIM):
  $$\text{SSIM}(x, y) = \frac{(2\mu_x\mu_y + C_1)(2\sigma_{xy} + C_2)}{(\mu_x^2 + \mu_y^2 + C_1)(\sigma_x^2 + \sigma_y^2 + C_2)}$$
- **Configurable Thresholds:**
  - Strict UI Primitives (Containers, Icons, Rectangles): $\text{SSIM} \ge 0.998$
  - Text & Subpixel Rasterization (FreeType vs. DirectWrite): $\text{SSIM} \ge 0.990$ (accommodating font gamma and antialiasing variance across OS drivers)
- Failed comparisons generate a composite side-by-side artifact: `[Golden Master] [Actual Output] [Amplified Pixel Difference]`.

---

# 5. Security & Robustness Fuzzing

To guarantee sandbox resilience:
1. **IPC Protocol Fuzzing:** `libFuzzer` mutates IPC message headers (magic bytes, version numbers, payload lengths) and streams millions of malformed packets to the host Capability Broker.
   - *Pass Condition:* The host parser cleanly rejects malformed packets with `PD_PLUGIN_ERR_INVALID_ARGUMENT`; zero host memory corruption or unhandled crashes.
2. **Permission Bypass Testing:** Automated tests attempt to invoke restricted methods (e.g. `camera.capture`) from a plugin context that omitted the permission from its `plugin.yaml`.
   - *Pass Condition:* Broker unconditionally returns `PD_PLUGIN_ERR_PERMISSION_DENIED` and logs an audit violation.
3. **Sandbox Escape Validation:** Child plugin processes attempt unauthorized file access (`/etc/shadow`, `C:\Windows\System32`), network socket creation, or parent process debugging.
   - *Pass Condition:* The operating system sandbox (Seccomp on Linux, Restricted Low-Integrity Token on Windows) blocks the syscall, immediately terminating the misbehaving child.

---

# 6. Performance Benchmark Targets & Methodology

Performance metrics are gathered on dedicated bare-metal reference hardware (not virtualized CI runners) to ensure reproducible timings.

### 6.1 Provisional Engineering Benchmark Targets

| Metric | Target Specification | Measurement Method |
|---|---|---|
| **60 Hz Frame Processing Time** | $\le 10.0\text{ ms}$ CPU+GPU compute | Sum of CPU UI build, paint recording, and GPU render pass (excluding VSync wait) |
| **60 Hz Presentation Interval** | $16.67\text{ ms} \pm 0.5\text{ ms}$ (P99) | Measured timestamp interval between consecutive buffer swap presentations |
| **120 Hz Frame Processing Time** | $\le 5.5\text{ ms}$ CPU+GPU compute | High-refresh compute budget ensuring no missed VSync ticks |
| **120 Hz Presentation Interval** | $8.33\text{ ms} \pm 0.3\text{ ms}$ (P99) | High-refresh interval stability |
| **Input-to-Presentation Latency** | $\text{P95} \le 25\text{ ms}, \text{P99} \le 35\text{ ms}$ | Measured from OS pointer/keyboard event arrival to display buffer swap |
| **Cold Startup Latency** | $\le 120\text{ ms}$ | Process launch (`main`) to first frame presented on screen |
| **Warm Startup Latency** | $\le 45\text{ ms}$ | Re-initialization from warm disk cache |
| **Subtree Rebuild Duration** | $\le 0.5\text{ ms}$ for 1,000 widgets | CPU time spent in `BuildOwner::flush_build()` |
| **Layout Calculation Cost** | $\le 1.0\text{ ms}$ for 1,000 widgets | CPU time spent in `RenderObject::layout()` pass |
| **Memory Footprint (Baseline)** | $\le 28\text{ MB}$ RSS | Idle desktop counter application |
| **Dedicated Heap Allocations** | $\le 12\text{ MB}$ active heap | Measured via allocator hooks (`malloc_info` / `HeapWalk`), excluding cached pages |
| **Dedicated GPU VRAM Usage** | $\le 45\text{ MB}$ VRAM | Queried via `VK_EXT_memory_budget` / `DXGI_QUERY_VIDEO_MEMORY_INFO` |
| **24-Hour Soak Stability** | Zero memory growth ($\Delta\text{Heap} \le 1\%$) | Continuous 60 FPS animation loop over 24 hours |

### 6.2 Profiling Instrument Breakdown & Multi-Dimensional Memory Tracking
PingDart strictly isolates performance and memory tracking into decoupled telemetry channels:
- **CPU UI Time:** Time spent in event dispatch, widget builds, and layout constraint solving.
- **CPU Paint Time:** Time spent recording the `DisplayList`.
- **GPU Render Time:** Time spent on GPU rasterization and composition (measured via `vkGetQueryPoolResults` / D3D12 timestamp queries).
- **Presentation Latency:** Time spent waiting for display refresh in `vkQueuePresentKHR` / `IDXGISwapChain::Present`.
- **Multi-Dimensional Memory Accounting:** Because OS Resident Set Size (RSS) fluctuates due to driver caches, allocator page retention, and OS paging, PingDart benchmarks simultaneously track:
  1. *Host Process RSS:* Operating system working set size.
  2. *Retained Heap Memory:* Active application-level allocations tracked via internal allocator arenas.
  3. *GPU VRAM Allocations:* Dedicated video memory allocated via VMA / D3D12MA.

---

# 7. Hardware Benchmark Reference Profiles

To prevent benchmark drift, official performance numbers are measured against two standardized hardware baselines:

### Profile A: Windows Desktop Baseline
- **CPU:** AMD Ryzen 7 5800X / Intel Core i7-12700K
- **GPU:** NVIDIA GeForce RTX 3060 (12 GB) / Intel Arc A770
- **Display:** 2560x1440 @ 144Hz (G-Sync / FreeSync enabled)
- **OS:** Windows 11 Pro (23H2)

### Profile B: Linux Desktop Baseline
- **CPU:** AMD Ryzen 5 5600G / Intel Core i5-12400
- **GPU:** Integrated AMD Radeon Graphics / Intel Iris Xe
- **Display:** 1920x1080 @ 60Hz
- **OS:** Ubuntu 22.04 LTS (Kernel 6.2, GNOME Wayland)

---

# 8. Release Quality Gates & Acceptance Policy

Every candidate release of PingDart must pass through three non-negotiable quality gates:

```text
+--------------------------------------------------------------------------+
| Gate 1: Pull Request Merge Gate                                          |
| - 100% C++ Unit Tests (GoogleTest) and Rust Tests (Cargo) pass           |
| - AddressSanitizer and UndefinedBehaviorSanitizer report zero issues     |
| - Clang-Tidy and formatting linters report zero warnings                 |
+--------------------------------------------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
| Gate 2: Nightly Continuous Verification Gate                             |
| - Full integration test matrix on Windows (Win32) and Linux (Wayland/X11)|
| - Vulkan & D3D12 validation layers report zero errors                    |
| - ThreadSanitizer data-race stress test passes                           |
| - IPC security fuzzing suite runs 500,000 iterations without crash       |
+--------------------------------------------------------------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
| Gate 3: Release Candidate (RC) Gate                                      |
| - 24-hour memory soak test completes without memory creep (leak-free)   |
| - Packaging test: MSIX and AppImage deploy and run on clean vanilla VMs  |
| - Performance benchmarks meet all P95/P99 latency and frame targets      |
| - Regression Gate: Statistically significant regression (> 5%, p < 0.01) |
+--------------------------------------------------------------------------+
```

### 8.1 Statistical Noise & Regression Evaluation Policy
To prevent false-positive CI failures from OS background tasks, CPU thermal throttling, or scheduler jitter:
1. **Repeated Executions:** Every benchmark scenario executes $N = 30$ iterations on dedicated, bare-metal baseline hardware.
2. **Outlier Filtering & Confidence Intervals:** The harness calculates the 95% Confidence Interval (CI) and interquartile range (IQR) across runs.
3. **Hypothesis Testing:** A performance regression is flagged as a blocking release failure if and only if:
   - The candidate's mean processing time is $> 5\%$ slower than the baseline, AND
   - A two-sample Welch's t-test / Mann-Whitney U test confirms statistical significance with $p < 0.01$.
4. **Per-Benchmark Thresholds:** Tighter thresholds ($\le 2\%$) apply to core runtime event loop dispatch; wider tolerances ($\le 5\%$) apply to complex GPU text rasterization.

---

# 9. Next Technical Step

With **DOC-07** establishing the complete testing, sanitation, and benchmarking strategy, the final specification in the architecture freeze series is **DOC-08: V1.0 Development Task Breakdown**, which will define:
1. Granular epic, feature, and task hierarchies with unique ticket IDs (`ENG-XXXX`).
2. Exact dependency graph across phases.
3. Sprint-by-sprint engineering milestones for Phase 1 Desktop delivery.

# PingDart Framework
## DOC-02: Rendering Engine Architecture Specification

**Status:** Technical Architecture Specification  
**Version:** 1.0  
**Language Baseline:** C++20  
**Parent Documents:** [PingDart PRD](prd.md) · [DOC-01: Core Runtime](doc-01-core-runtime-architecture.md) · [DOC-03: Platform Adapter SDK](doc-03-platform-adapter-sdk.md)  
**Target Phase:** Phase 1 (Desktop Proof-of-Concept)

---

# 1. Executive Summary

The **Rendering Engine** is the visual execution core of PingDart. It consumes high-level widget paint requests and transforms them into high-performance, GPU-accelerated frames delivered to native surfaces provided by DOC-03.

For **Phase 1**, this specification establishes:
1. The **PingDart Rendering Hardware Interface (RHI)**: an abstract, modern GPU hardware interface decoupling the engine from specific graphics APIs.
2. The **Vulkan Backend**: the primary cross-platform graphics pipeline for **Linux (Wayland & X11)** and **Windows**.
3. The **Direct3D 12 Backend**: the first-class native graphics pipeline for **Windows 10 and 11**.
4. The **Display List & Compositor**: immutable recording of 2D drawing primitives, transforms, clipping, and dirty region tracking.
5. The **Skia Evaluation Strategy**: vector rasterization and text rendering integration without coupling Skia to platform host code.
6. **Frame Scheduling & Presentation**: VSync synchronization, frame pacing, and dropped-frame recovery.
7. **GPU Resilience**: handling window resizing, surface invalidation, and GPU device loss.

```text
+--------------------------------------------------------------------------+
|                     Widget & Layout Engine (DOC-05)                      |
|                Render Tree · Layout · Invalidation Flags                 |
+--------------------------------------------------------------------------+
                                     |
+--------------------------------------------------------------------------+
|                        Display List & Compositor                         |
|     Immutable Drawing Commands · Layers · Transforms · Damage Regions    |
+--------------------------------------------------------------------------+
                                     |
+--------------------------------------------------------------------------+
|                PingDart Rendering Hardware Interface (RHI)               |
|                                                                          |
|  [IRenderDevice]      [ICommandBuffer]       [ITexture]                  |
|  [IRenderSurface]     [ISwapchain]           [IPipelineState]            |
+--------------------------------------------------------------------------+
                  |                                      |
                  v                                      v
+-----------------------------------+  +-----------------------------------+
|          Vulkan Backend           |  |         Direct3D 12 Backend       |
|          (Linux & Windows)        |  |              (Windows)            |
|  - VkInstance, VkPhysicalDevice   |  |  - IDXGIFactory7, IDXGIAdapter4   |
|  - WSI (Wayland, XCB, Win32)      |  |  - ID3D12Device8, Command Queues  |
|  - VkSwapchainKHR (Mailbox/FIFO)  |  |  - DXGI Flip-Model Swapchain      |
|  - VMA (Vulkan Memory Allocator)  |  |  - D3D12MA (Memory Allocator)     |
+-----------------------------------+  +-----------------------------------+
                                     |
+--------------------------------------------------------------------------+
|                Native Surface Delivery from DOC-03 (PAL)                 |
|                   (Win32 HWND, Wayland wl_surface, X11 Window)           |
+--------------------------------------------------------------------------+
```

---

# 2. Rendering Hardware Interface (RHI) Contracts

All RHI interfaces reside in `pingdart::rhi`, use C++20 standard conventions, and return value-based `pingdart::core::Result<T>` values.

### 2.1 Common RHI Types (`pingdart/rhi/rhi_types.h`)

```cpp
#pragma once
#include <cstdint>
#include <string_view>

namespace pingdart::rhi {

enum class GraphicsApi : uint8_t {
    Vulkan,
    Direct3D12,
    Metal,      // Future (macOS / iOS)
    WebGPU,     // Future (Web)
    Software
};

enum class PixelFormat : uint8_t {
    Unknown,
    RGBA8_UNORM,
    BGRA8_UNORM,
    RGBA16_FLOAT,
    D24_UNORM_S8_UINT,
    D32_FLOAT
};

enum class PresentMode : uint8_t {
    Immediate,  // Tearing allowed, lowest latency
    Fifo,       // VSync locked (Double/Triple buffered, guaranteed tear-free)
    Mailbox     // Triple-buffered low latency without tearing (uncapped frames)
};

enum class TextureUsage : uint32_t {
    Sampled         = 1 << 0,
    RenderTarget    = 1 << 1,
    DepthStencil    = 1 << 2,
    TransferSrc     = 1 << 3,
    TransferDst     = 1 << 4
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
    return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool operator&(TextureUsage a, TextureUsage b) {
    return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

struct Viewport {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
    float min_depth{0.0f};
    float max_depth{1.0f};
};

struct ScissorRect {
    int32_t x{0};
    int32_t y{0};
    uint32_t width{0};
    uint32_t height{0};
};

struct ColorClearValue {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{1.0f};
};

} // namespace pingdart::rhi
```

### 2.2 RHI Core Interfaces (`pingdart/rhi/*.h`)

#### A. `ITexture` & `ISwapchain` (`pingdart/rhi/i_texture.h`, `pingdart/rhi/i_swapchain.h`)
```cpp
#pragma once
#include "rhi_types.h"
#include <pingdart/core/result.h>
#include <cstdint>

namespace pingdart::rhi {

class ITexture {
public:
    virtual ~ITexture() = default;

    [[nodiscard]] virtual uint32_t width() const noexcept = 0;
    [[nodiscard]] virtual uint32_t height() const noexcept = 0;
    [[nodiscard]] virtual PixelFormat format() const noexcept = 0;
    [[nodiscard]] virtual TextureUsage usage() const noexcept = 0;
};

class ISwapchain {
public:
    virtual ~ISwapchain() = default;

    // Resizes swapchain backbuffers when native surface changes
    [[nodiscard]] virtual core::Status resize(uint32_t width, uint32_t height) = 0;

    // Acquires the next available drawable backbuffer
    [[nodiscard]] virtual core::Result<ITexture*> acquire_next_backbuffer() = 0;

    // Submits the rendered backbuffer to display engine (VSync presentation)
    [[nodiscard]] virtual core::Status present(PresentMode mode) = 0;

    [[nodiscard]] virtual uint32_t width() const noexcept = 0;
    [[nodiscard]] virtual uint32_t height() const noexcept = 0;
    [[nodiscard]] virtual PixelFormat format() const noexcept = 0;
    [[nodiscard]] virtual uint32_t buffer_count() const noexcept = 0;
};

} // namespace pingdart::rhi
```

#### B. `ICommandBuffer` (`pingdart/rhi/i_command_buffer.h`)
```cpp
#pragma once
#include "rhi_types.h"
#include <pingdart/core/result.h>

namespace pingdart::rhi {

class ITexture;
class IPipelineState;

class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;

    virtual void begin() = 0;
    virtual void end() = 0;

    virtual void begin_render_pass(
        ITexture* render_target,
        const ColorClearValue& clear_color) = 0;
    virtual void end_render_pass() = 0;

    virtual void set_viewport(const Viewport& viewport) = 0;
    virtual void set_scissor(const ScissorRect& scissor) = 0;

    virtual void bind_pipeline(IPipelineState* pipeline) = 0;

    virtual void draw(uint32_t vertex_count, uint32_t first_vertex = 0) = 0;
    virtual void draw_indexed(uint32_t index_count, uint32_t first_index = 0, int32_t vertex_offset = 0) = 0;
};

} // namespace pingdart::rhi
```

#### C. `IRenderDevice` (`pingdart/rhi/i_render_device.h`)
```cpp
#pragma once
#include "rhi_types.h"
#include <pingdart/core/result.h>
#include <pingdart/pal/native_types.h>
#include <memory>

namespace pingdart::rhi {

class ISwapchain;
class ITexture;
class ICommandBuffer;
class IPipelineState;

struct RenderDeviceDescriptor {
    GraphicsApi preferred_api{GraphicsApi::Vulkan};
    bool enable_validation_layers{false};
    bool prefer_discrete_gpu{true};
};

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    [[nodiscard]] virtual GraphicsApi api_type() const noexcept = 0;
    [[nodiscard]] virtual std::string_view device_name() const noexcept = 0;

    // Creates a presentation swapchain from DOC-03 native surface handle
    [[nodiscard]] virtual core::Result<std::unique_ptr<ISwapchain>> create_swapchain(
        const pal::NativeSurfaceHandle& surface_handle,
        uint32_t width,
        uint32_t height,
        PresentMode preferred_mode = PresentMode::Fifo) = 0;

    // Creates an off-screen texture
    [[nodiscard]] virtual core::Result<std::unique_ptr<ITexture>> create_texture(
        uint32_t width,
        uint32_t height,
        PixelFormat format,
        TextureUsage usage) = 0;

    // Creates an executable command buffer
    [[nodiscard]] virtual core::Result<std::unique_ptr<ICommandBuffer>> create_command_buffer() = 0;

    // Submits command buffers to primary graphics queue
    virtual void submit(ICommandBuffer* command_buffer) = 0;

    // Synchronizes host with GPU completion (clean shutdown/reset)
    virtual void wait_idle() = 0;

    // Static Factory
    static core::Result<std::unique_ptr<IRenderDevice>> create(const RenderDeviceDescriptor& desc);
};

} // namespace pingdart::rhi
```

---

# 3. Vulkan Backend Architecture (Linux & Windows)

The Vulkan backend is the primary cross-platform GPU driver for Linux (Wayland/X11) and modern Windows.

### 3.1 Instance Creation & WSI Extensions
1. **API Version:** Targets `VK_API_VERSION_1_2` minimum (providing timeline semaphores, descriptor indexing, and buffer device address).
2. **Validation Layers:** Automatically enabled in Debug builds (`VK_LAYER_KHRONOS_validation`). A custom debug messenger callback logs GPU errors directly to the PingDart logger.
3. **Window System Integration (WSI) Extensions:**
   - Universal: `VK_KHR_surface`
   - Windows: `VK_KHR_win32_surface`
   - Linux Wayland: `VK_KHR_wayland_surface`
   - Linux X11: `VK_KHR_xcb_surface` or `VK_KHR_xlib_surface`

### 3.2 Physical Device Selection & Queue Families
1. **Device Scoring:** Enumerates all `VkPhysicalDevice` candidates, scoring discrete GPUs (e.g. NVIDIA, AMD, Intel Arc) higher than integrated GPUs.
2. **Queue Family Discovery:** Locates a single queue family supporting both `VK_QUEUE_GRAPHICS_BIT` and WSI surface presentation (`vkGetPhysicalDeviceSurfaceSupportKHR`). If present and graphics queues differ, the backend uses `VK_SHARING_MODE_CONCURRENT`.

### 3.3 Swapchain Lifecycle & Synchronization
1. **Surface Creation:**
   - On Windows: `vkCreateWin32SurfaceKHR(instance, &win32_info, nullptr, &surface)`
   - On Wayland: `vkCreateWaylandSurfaceKHR(instance, &wayland_info, nullptr, &surface)`
   - On X11: `vkCreateXcbSurfaceKHR(instance, &xcb_info, nullptr, &surface)`
2. **Double / Triple Buffering:**
   - Queries `minImageCount` and requests `std::clamp(minImageCount + 1, minImageCount, maxImageCount)` (typically 3 for Mailbox, 2 for FIFO).
3. **Synchronization Semaphores:**
   - `VkSemaphore image_acquired_semaphore`: signaled by `vkAcquireNextImageKHR`.
   - `VkSemaphore render_finished_semaphore`: signaled by queue submission, waited on by `vkQueuePresentKHR`.
   - `VkFence in_flight_fence`: per frame-in-flight (typically 2 frames) to prevent CPU from outpacing GPU.
4. **Suboptimal & Out-of-Date Recovery:**
   - If `vkAcquireNextImageKHR` or `vkQueuePresentKHR` returns `VK_ERROR_OUT_OF_DATE_KHR` or `VK_SUBOPTIMAL_KHR`, the backend marks the swapchain dirty and triggers an immediate recreation pass matching the new native surface dimensions.

---

# 4. Direct3D 12 Backend Architecture (Windows)

For Windows 10/11 environments, Direct3D 12 provides zero-overhead GPU access and optimal Windows composition integration.

### 4.1 DXGI Adapter & Device Selection
1. **DXGI Factory:** Creates `IDXGIFactory7`. In Debug builds, enables `ID3D12Debug::EnableDebugLayer()`.
2. **Adapter Enumeration:** Enumerates `IDXGIAdapter4` using `EnumAdapterByGpuPreference(..., DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE)`.
3. **Device Creation:** Calls `D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))`.

### 4.2 Command Queues & Descriptor Heaps
1. **Command Queues:** Instantiates a direct command queue (`D3D12_COMMAND_LIST_TYPE_DIRECT`).
2. **Descriptor Heaps:** Allocates dedicated heaps:
   - Render Target Views (RTV Heap): `D3D12_DESCRIPTOR_HEAP_TYPE_RTV` (sized for swapchain backbuffers).
   - Shader Resource Views (SRV Heap): `D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV` (shader-visible).

### 4.3 DXGI Flip-Model Swapchain
1. **Creation:** Invokes `IDXGIFactory2::CreateSwapChainForHwnd` with:
   - `DXGI_SWAP_EFFECT_FLIP_DISCARD` (mandatory modern flip presentation model).
   - Format: `DXGI_FORMAT_R8G8B8A8_UNORM` or `DXGI_FORMAT_B8G8R8A8_UNORM`.
   - Buffer Count: 2 or 3.
   - Flags: `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` (for variable refresh rate / G-Sync).
2. **Resource Barriers:** Explicit state transitions:
   - Before draw: `D3D12_RESOURCE_STATE_PRESENT` $\to$ `D3D12_RESOURCE_STATE_RENDER_TARGET`.
   - Before present: `D3D12_RESOURCE_STATE_RENDER_TARGET` $\to$ `D3D12_RESOURCE_STATE_PRESENT`.

---

# 5. Display List & 2D Compositor Pipeline

PingDart does not paint immediately to the GPU during widget tree traversal. It records painting operations into an **immutable Display List**, decoupling layout logic from rasterization.

### 5.1 Immutable Drawing Commands (`DisplayList`)

```text
Widget Tree Layout
        |
    paint()
        |
+-------------------------------------------------------+
|                 DisplayListBuilder                    |
|                                                       |
|  - push_clip_rect(rect, ClipOp::Intersect)            |
|  - push_transform(matrix4x4)                          |
|  - draw_rounded_rect(rrect, paint)                    |
|  - draw_text_blob(text, font, origin, paint)          |
|  - draw_image(texture, dest_rect, paint)              |
|  - pop_transform()                                    |
|  - pop_clip()                                         |
+-------------------------------------------------------+
        |
  finish_recording()
        |
+-------------------------------------------------------+
|             Immutable DisplayList Object              |
|       (Can be retained, replayed, or cached)          |
+-------------------------------------------------------+
```

### 5.2 Dirty Regions & Partial Redraw Optimization
- Every widget specifies its damage bounding rectangle (`Rect`).
- If only a blinking cursor or button hover changes, the compositor computes the **accumulated damage region** (`DamageRegion = Union(DirtyRects)`).
- The compositor scissors the GPU rasterization pass to the damage region, eliminating full-screen redraw overhead.

### 5.3 UI/Render Thread Boundary & DisplayList Handoff (Fixed-Slot Mailbox)
To guarantee thread-safe publication, zero lock contention, and provably safe object reclamation:
1. **Immutable DisplayList Lifecycle:** The UI thread records draw commands into `DisplayListBuilder` during the paint phase. Calling `finish_recording()` freezes the recorded commands into an immutable `core::RefPtr<DisplayList>`.
2. **Fixed-Slot Mailbox Architecture:**
   - Rather than relying on a naive raw atomic pointer (which risks race conditions during concurrent read/reclaim), PingDart uses a dedicated **Triple-Buffered Mailbox** (`DisplayListMailbox` with 3 discrete slots).
   - Each slot holds:
     - `core::RefPtr<DisplayList> display_list`
     - `std::atomic<uint64_t> sequence_number{0}` (monotonically increasing)
     - `std::atomic<uint8_t> state` (`Empty`, `Writing`, `Ready`, `Rendering`, `Retired`)
3. **Explicit Ownership Transfer & Replay Contract:**
   - **Producer (UI Thread):** Selects an `Empty` or `Retired` slot $\to$ sets state `Writing` $\to$ assigns `RefPtr<DisplayList>` $\to$ stamps `sequence_number` $\to$ atomically publishes state to `Ready`.
   - **Consumer (Render Thread):** Checks mailbox for highest `sequence_number` in `Ready` state $\to$ atomically transitions to `Rendering` $\to$ acquires local reference $\to$ executes GPU rasterization $\to$ upon frame completion, transitions slot to `Retired` and releases reference.
   - **Safe Reclamation:** The UI thread never overwrites or reclaims a slot currently marked `Rendering`. If the render thread is delayed, older intermediate `Ready` slots are retired, skipping stale frames without CPU or GPU memory leaks.
4. **Strict GPU Resource Ownership:** All physical GPU resources (`VkDevice`, `ID3D12Device`, swapchains, descriptor heaps, textures) are strictly owned by the **Render Thread**. The UI thread never directly invokes GPU APIs or manipulates hardware handles.

---

# 6. Skia Integration Strategy

Skia is evaluated as the 2D vector drawing and text rendering engine without creating architectural dependency on Skia's windowing abstractions:

```text
+-------------------------------------------------------+
|                 PingDart Display List                 |
+-------------------------------------------------------+
                           |
                     Rasterizer Pass
                           |
+-------------------------------------------------------+
|              Skia 2D Rasterization Engine             |
|        (Path rasterization, HarfBuzz text shaping)    |
+-------------------------------------------------------+
                           |
            GrDirectContext::MakeVulkan(...)
                           |
+-------------------------------------------------------+
|               PingDart RHI Vulkan Texture             |
|                  (Swapchain Backbuffer)               |
+-------------------------------------------------------+
```

1. **Context Initialization:** On initialization, DOC-02 wraps the RHI's `VkDevice` and queues into a `GrDirectContext` (`GrDirectContext::MakeVulkan`).
2. **Backbuffer Wrapping & Validation:**
   - *Technical Constraint:* Zero-copy wrapping of Vulkan swapchain images via `SkSurfaces::WrapBackendRenderTarget` has driver-specific synchronization and image layout constraints (`VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`).
   - PingDart treats zero-copy wrapping as an **evaluated capability, not an unconditional guarantee**.
   - If driver constraints or swapchain image usage flags prohibit zero-copy wrapping, DOC-02 falls back to rendering into an offscreen render target and executing a GPU blit / copy into the presentable swapchain backbuffer.
3. **Decoupled Boundary:** If Skia is replaced in future phases (e.g., with a custom compute-based rasterizer like Vello), application-level code and the DOC-03 PAL remain 100% untouched.

---

# 7. Frame Scheduling & Presentation Sync

PingDart strictly avoids tearing, micro-stuttering, and buffer starvation via synchronized frame pacing.

```text
VSync Signal (OS / Compositor)
       |
+------v------------------------------------------------+
| Frame Phase 1: Begin Frame (Timestamp, Latency Target)|
+-------------------------------------------------------+
       |
+------v------------------------------------------------+
| Frame Phase 2: Animate & Layout (UI Thread)           |
+-------------------------------------------------------+
       |
+------v------------------------------------------------+
| Frame Phase 3: Record Display List                    |
+-------------------------------------------------------+
       |  (Hand off to Render Thread)
+------v------------------------------------------------+
| Frame Phase 4: Rasterize & GPU Command Submit         |
+-------------------------------------------------------+
       |
+------v------------------------------------------------+
| Frame Phase 5: Present (vkQueuePresentKHR / Present1) |
+-------------------------------------------------------+
```

### 7.1 Latency Budgeting
- **60 Hz Display:** 16.6 ms frame budget.
- **120 Hz Display:** 8.3 ms frame budget.
- If UI layout exceeds budget, the frame scheduler drops intermediate frames cleanly without piling up queue backlog.

---

# 8. GPU Memory Management

1. **Memory Allocator Integration:**
   - Vulkan: Uses **VMA (Vulkan Memory Allocator)** to pool small buffers into larger `VkDeviceMemory` allocations, avoiding the 4,096 platform allocation limit.
   - Direct3D 12: Uses **D3D12MA (D3D12 Memory Allocator)** for heap pooling.
2. **Resource Lifetimes & Fences:**
   - GPU resources scheduled for release (`release()`) are queued in a deferred destruction ring buffer held for `NUM_FRAMES_IN_FLIGHT` (2 frames). Only when the associated fence signals completion are resources reclaimed.
3. **Texture Cache Eviction:**
   - An LRU (Least Recently Used) texture cache manages decoded images with an eviction budget (default: 256 MB desktop, 64 MB mobile/wearable).

---

# 9. Failure Recovery & Resilience

### 9.1 Surface Loss & Window Resize
- When `IWindow` triggers `WindowLifecycleEvent::WindowResized`, the swapchain marks itself stale.
- On the next frame, the render thread waits for GPU idle (`wait_idle()`), queries the new dimensions from DOC-03's `ISurfaceProvider::physical_size()`, and recreates the swapchain using the existing surface token.

### 9.2 GPU Device Loss & Fallback Architecture
- **Device Loss Detection:** Catches `VK_ERROR_DEVICE_LOST` (Vulkan) or `DXGI_ERROR_DEVICE_REMOVED` / `DXGI_ERROR_DEVICE_RESET` (D3D12).
- **Full Re-instantiation Pipeline:**
  1. Tear down the dead `IRenderDevice`, command queues, descriptor heaps, pipelines, and swapchains.
  2. Query `ID3D12Device::GetDeviceRemovedReason()` or Vulkan diagnostic status.
  3. Re-enumerate system GPU adapters to select a healthy physical device.
  4. Reconstruct `IRenderDevice`, recompile pipeline states, and notify DOC-05 to replay the active display list.
- **Dedicated Software Fallback Presentation Path:**
  - If hardware acceleration fails repeatedly or no supported GPU is found, the engine drops into a **separately tested CPU software presentation path**:
    - **Windows:** CPU software rasterizer renders into an RGBA32 DIB section presented directly to the HWND via `SetDIBitsToDevice` / `BitBlt`.
    - **Linux Wayland:** Allocates shared-memory buffers via `wl_shm` / `wl_shm_pool`, presenting CPU pixel buffers to `wl_surface_attach`.
    - **Linux X11:** Uses `XCreateImage` and `XPutImage` to blit software framebuffers to the window.

---

# 10. Recommended Implementation Sequence

To deliver a working rendering engine safely and incrementally:

```text
Step 1: Define RHI Interfaces & Types (i_render_device.h, i_swapchain.h, etc.)
                   |
Step 2: Build Vulkan WSI Swapchain on Windows & Linux (doc-03 native surface)
                   |
Step 3: Render Clear-Color Frame (Proof of GPU Presentation)
                   |
Step 4: Render Vertex Triangle & Textured Quad (Proof of Pipeline & Shader Binding)
                   |
Step 5: Resizing, VSync Pacing & Clean Device Shutdown
                   |
Step 6: Build D3D12 Backend on Windows
                   |
Step 7: Skia Integration & Display List Rasterizer
                   |
Step 8: Connect Rendering Pipeline to DOC-01 Scheduler & DOC-03 Surface Events
```

### Initial Milestone Deliverable:
A cross-platform C++ application running on **Windows and Linux** that:
1. Opens a native window using DOC-03.
2. Creates a Vulkan swapchain.
3. Renders a smooth animated clear-color / triangle loop at 60 FPS.
4. Handles dynamic window resizing without crashes or tearing.
5. Shuts down cleanly with zero GPU memory leaks or Vulkan validation layer warnings.

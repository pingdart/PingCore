# PingDartUI — Unified UI Component & Styling Framework
## 200% Cross-Platform Native Master Engineering Architecture & Implementation Blueprint

**Document ID**: `PDU-ARCH-MASTER-2026-V1`  
**Date**: November 18, 2026  
**Status**: **APPROVED FOR EXECUTION (PKG-135 FOUNDATION INITIALIZED)**  
**Target Platforms**: Android (AArch64), iOS (arm64 Metal), macOS (arm64/x86_64 Metal), Windows (x86_64 Direct3D 12), Linux (x86_64/aarch64 Wayland/X11), Web (SSR/WASM/HTML5)  
**Core Technologies**: C++20, Internal Qt 6 Adapter, Direct PingCore RHI, PingDart CSS Engine, Reactive Signals, Next.js-Style File Router  

---

## 1. Executive Vision & Core Principles

**PingDartUI** provides developers with the developer ergonomics and expressive declarative syntax of modern frontend frameworks (React, Next.js, SwiftUI), combined with the raw execution speed, deterministic memory profile, and native GPU rendering of modern C++20.

### Core Tenets:
1. **Zero Raw Pointer Exposure**: No `std::unique_ptr`, no `new`/`delete`, no raw memory management for application developers. Components are lightweight, value-type handles (`Component`) backed by copy-on-write internal nodes.
2. **Pluggable Multi-Renderer Isolation**: Qt 6 is strictly an internal rendering adapter (`@pingdart/ui-qt`). Not a single Qt header is leaked into `<pingdart/ui.h>`. Projects can render via Qt Quick, direct native RHI (Metal/Vulkan/Direct3D), or Web SSR without modifying a line of application code.
3. **Three-Way Unified Styling**: Normal CSS files, Tailwind utility classes, and direct fluent C++ method chains all compile into the identical internal `ComputedStyle` representation.
4. **Single Codebase for All 6 OS**: Write the UI component once; deploy natively to Android, iOS, macOS, Windows, Linux, and Web.
5. **Future-Proof `.pdc` Language**: The C++ declarative component AST is 100% isomorphic to the upcoming `.pdc` (PingDart Component) language syntax.

---

## 2. Developer-Facing Syntax & Ergonomics

### The Proposed "DoctorCard" Component:
```cpp
#include <pingdart/ui.h>
using namespace pingdart::ui;

Component DoctorCard() {
    auto isBooking = useState<bool>(false);

    return Card({
        Heading("Dr. Jane Smith, MD").size(24).weight(FontWeight::Bold),
        Text("Cardiologist").color("#2563EB").weight(FontWeight::Medium),
        Text("15 years clinical experience").color("#64748B"),
        Text("Heart Care Clinic · San Francisco, CA").color("#64748B"),
        
        Row({
            Button("Book Consultation")
                .variant(ButtonVariant::Primary)
                .background("#2563EB")
                .color("#FFFFFF")
                .borderRadius(8)
                .padding(12, 20)
                .onClick([isBooking]() mutable {
                    isBooking.set(true);
                    Router::push("/booking/dr-smith");
                }),
            Link("View Full Profile", "/doctor/jane-smith")
                .color("#2563EB")
                .padding(12, 16)
        }).gap(12).alignItems(AlignItems::Center)
    })
    .className("doctor-card")
    .background("#FFFFFF")
    .border(1, "#E2E8F0")
    .borderRadius(16)
    .padding(24)
    .gap(12)
    .shadow(0, 4, 12, "#00000010");
}
```

---

## 3. The Three-Way Styling System

All three styling modalities parse into an optimized, compact C++ struct: `ComputedStyle`.

* **Method A: Standard CSS Parser**: Loads external `.css` files, resolves rules against class names.
* **Method B: Tailwind Utility Classes**: Parses `className("bg-blue-600 text-white rounded-xl p-4 gap-3")` into layout, margin, padding, border, and color properties.
* **Method C: Fluent C++ Chaining**: Directly sets properties via chained calls `.background(...)`, `.padding(...)`, etc.

---

## 4. 70-Component Complete Catalog

The initial PingDartUI library provides 70 production-grade components across 12 categories:
1. **Basic**: `Text`, `Heading`, `Image`, `Icon`, `Divider`, `Spacer`
2. **Layout**: `Container`, `Row`, `Column`, `Stack`, `Flex`, `Grid`, `ScrollView`
3. **Buttons**: `Button`, `IconButton`, `FloatingButton`, `ButtonGroup`
4. **Forms**: `Input`, `TextArea`, `Checkbox`, `Radio`, `Switch`, `Select`, `Slider`
5. **Date & Time**: `DatePicker`, `TimePicker`, `Calendar`
6. **Navigation**: `Navbar`, `Sidebar`, `Tabs`, `Drawer`, `Breadcrumb`, `BottomNavigation`
7. **Data Display**: `Card`, `Avatar`, `Badge`, `Table`, `DataGrid`, `List`, `Tree`
8. **Feedback**: `Alert`, `Toast`, `Snackbar`, `ProgressBar`, `Spinner`, `Skeleton`
9. **Overlays**: `Modal`, `Dialog`, `Popover`, `Tooltip`, `BottomSheet`
10. **Advanced**: `Charts`, `RichTextEditor`, `VirtualList`, `FilePicker`
11. **Application**: `AppShell`, `Page`, `Router`, `Outlet`, `SafeArea`
12. **Media**: `VideoPlayer`, `AudioPlayer`, `ImageGallery`

---

## 5. 200% Native OS Execution Matrix

| Platform | Rendering Architecture | Native Pipeline | Output Binary |
|---|---|---|---|
| **Android** | Qt Quick / PingCore RHI | SurfaceView + ANativeWindow + Vulkan/GLES | APK / AAB |
| **iOS** | Qt Quick / PingCore RHI | CAMetalLayer + Metal 3 GPU family | .app / IPA |
| **macOS** | Qt Quick / PingCore RHI | NSWindow + CAMetalLayer + Apple Silicon GPU | Mach-O / .app |
| **Windows** | Qt Quick / PingCore RHI | HWND + Direct3D 12 Swapchain + DXGI | PE/COFF .exe |
| **Linux** | Qt Quick / PingCore RHI | Wayland xdg_shell / X11 + Vulkan/GL | ELF64 / AppImage |
| **Web** | PingCore Web Engine | SSR HTML + CSS3 + DOM Hydration + WASM | Static + Client JS/WASM |

---

## 6. Official Modular Package Ecosystem

```text
@pingdart/
├── ui                 (Public Component API, State, Virtual DOM)
├── ui-qt              (Qt 6.7+ QQuickRenderControl & SceneGraph adapter)
├── ui-native          (PingCore RHI Direct Metal / Vulkan / D3D12 adapter)
├── ui-web             (HTML5 / CSS3 / SSR / Hydration / WASM adapter)
├── css                (Pure C++ CSS3 Lexer, Parser & Cascade Engine)
├── utility-css        (Tailwind-compatible utility compiler)
├── icons              (Lucide-compatible cross-platform vector icons)
├── animations         (Spring, timing, and keyframe animation drivers)
├── forms              (Validation schema, form controller, dirty tracking)
└── themes             (Design token system, dark mode, system accent sync)
```

---

## 7. Phased Implementation Roadmap (PKG-135 through PKG-143)

* **PKG-135 (Now Active)**: PingDartUI Foundation — Component value-type handle, `useState`, `ComputedStyle`, `IRenderAdapter` interface, `DoctorCard` demo, and test suite.
* **PKG-136**: PingDart CSS & Tailwind Engine — Full CSS parser, token cascade, Flexbox layout math.
* **PKG-137**: 70 Core Components — Full implementation of all 70 components with accessibility.
* **PKG-138**: Reactive Engine & Gestures — PingCore signal integration, reconciliation, drag/swipe gestures.
* **PKG-139**: Native Hardware Adapters — Qt 6 QQuickItem adapter, Metal 3, Vulkan 1.3, Direct3D 12.
* **PKG-140**: Web SSR & SEO — Chromium browser validation, hydration, SEO meta tags.
* **PKG-141**: Cross-Platform Universal Apps — Real-world `DoctorCard` and `MedicalDashboard` running across all 6 OS.
* **PKG-142**: PDC Compiler — Native `.pdc` declarative language compiler generating C++ Component IR.
* **PKG-143**: PingDart Studio — Visual drag-and-drop designer and hot reload previewer.

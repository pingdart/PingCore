# PingCore

**PingCore** (PingDart Framework Engine) is a modern, high-performance C++20 cross-platform application framework and declarative UI runtime. Built from first principles, it provides a unified architecture for developing desktop, mobile, and embedded graphical applications with zero external framework dependencies.

---

## Key Highlights

- **Declarative Reactive UI**: C++20 widget composition with deterministic keyed reconciliation, stateful lifecycles, and Box/Flex constraint layouts.
- **Unified Platform Abstraction (PAL)**: Native windowing, lifecycle management, and hardware event dispatch across:
  - **macOS** (AppKit / Metal)
  - **Linux** (Wayland / X11)
  - **Windows** (Win32 / Vulkan)
  - **Android** (NDK Native Activity / `ANativeWindow` lifecycle fences)
  - **iOS** (UIKit / `CAMetalLayer` dynamic safe area insets)
- **Render Hardware Interface (RHI)**: Low-overhead abstraction over **Vulkan 1.3** and **Metal**, featuring swapchain management, pipeline caches, and physical GPU presentation.
- **Multilingual Typography Engine**:
  - Integrated **FreeType** glyph rendering and **HarfBuzz** complex text shaping (Arabic, Indic/Telugu/Hindi, Latin, CJK).
  - Skyline bin-packing glyph atlas with LRU GPU texture caching and fence-aware eviction.
- **Integrated Package Manager & Tooling (`pd`)**:
  - Semantic versioning engine (`^`, `~`, exact, ranges).
  - Cryptographic lockfile verification (`pingdart.lock` with SHA-256 integrity digests).
  - Automatic CMake target generation (`.pingdart/pingdart_packages.cmake`).
  - Standalone application packaging (`.app`, AppImage/AppDir, Windows bundles) with nested inside-out code signing.

---

## Repository Structure

```
PingDartCore/
├── include/pingdart/         # Public C++20 API headers
│   ├── core/                 # Event loop, ref counting, error types, compiler FFI
│   ├── pal/                  # Platform abstraction layer (macOS, Linux, Win, Android, iOS)
│   ├── rhi/                  # Render hardware interface (Vulkan, Metal, swapchains)
│   ├── render/               # Display list, 2D vector primitives, rasterization
│   ├── text/                 # HarfBuzz text shaping, FreeType rasterizer, glyph atlas
│   ├── widget/               # Declarative widgets, reconciliation, layout engine
│   ├── cli/                  # Packager, verifier, build orchestrator
│   └── pkg/                  # Package manager, SemVer resolver, lockfile, registry client
├── src/                      # Subsystem implementations
├── tools/pd/                 # 'pd' CLI tool source
├── examples/                 # Sample applications and widget showcases
├── tests/                    # 40+ automated CTest suites (GTest)
└── cmake/                    # CMake configuration modules and toolchains
```

---

## Building from Source

### Prerequisites
- **Compiler**: Clang 16+ or GCC 13+ with C++20 support
- **Build System**: CMake 3.25+ and Ninja
- **Graphics/Fonts**: Vulkan SDK (optional: Metal on macOS), FreeType 2.12+, HarfBuzz 8.0+

### Compilation
```bash
# Configure with CMake
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build core libraries, CLI tool, and tests
cmake --build build

# Run full automated test suite (40 test suites)
ctest --test-dir build --output-on-failure
```

---

## The `pd` Developer Tool

PingCore includes the unified `pd` tool for project creation, dependency management, and packaging:

```bash
# Inspect system environment and graphics toolchains
pd doctor

# Create a new reusable package
pd create package my-package

# Install dependencies from registry
pd add pingdart-charts

# List installed packages and lockfile resolution
pd list

# Build and package standalone application bundle
pd build
pd package --target macos
pd verify dist/MyApp.app
```

---

## License

PingCore is licensed under the Apache 2.0 License.

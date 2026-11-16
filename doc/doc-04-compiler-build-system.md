# PingDart Framework
## DOC-04: Compiler & Build System Architecture Specification

**Status:** Technical Architecture Specification  
**Version:** 1.0  
**Language Baseline:** C++20 · CMake 3.26+ · Ninja 1.11+  
**Parent Documents:** [PingDart PRD](prd.md) · [DOC-01](doc-01-core-runtime-architecture.md) · [DOC-03](doc-03-platform-adapter-sdk.md) · [DOC-02](doc-02-rendering-engine-architecture.md) · [DOC-05](doc-05-widget-layout-engine.md)  
**Target Phase:** Phase 1 (Desktop Proof-of-Concept)

---

# 1. Executive Summary

The **Compiler & Build System** defines the infrastructure required to build, test, link, and package PingDart applications across all supported operating systems.

The build architecture is bifurcated into two strictly segregated pipelines:
1. **The Framework Build Pipeline:** Compiles the PingDart engine libraries (`pingdart_core`, `pingdart_pal`, `pingdart_rhi`, `pingdart_ui`) from source using modern CMake and Ninja.
2. **The Application Build Pipeline:** Orchestrated by the official `pd` CLI, this pipeline consumes developer application source code, links prebuilt or locally compiled framework targets, packages assets, and produces distribution-ready platform binaries (Windows EXE/MSIX, Linux AppImage/Flatpak).

```text
+--------------------------------------------------------------------------+
|                     APPLICATION BUILD PIPELINE                           |
|       Developer Application Code (C++20 / Future Dedicated Syntax)       |
+--------------------------------------------------------------------------+
                                     |
                       PingDart CLI (`pd` Orchestrator)
                       [pd doctor, init, build, run, package]
                                     |
              +----------------------+----------------------+
              |                                             |
              v                                             v
+-----------------------------+               +----------------------------+
|     Host C++ Toolchains     |               |    PingDartCore Subsystem  |
|  - Clang 16+ / MSVC v143+   |               |  - Rust Compiler Engine    |
|  - CMake 3.26+ & Ninja      |               |  - Stable C-ABI Contract   |
+-----------------------------+               +----------------------------+
              |                                             |
              +----------------------+----------------------+
                                     |
                                     v
+--------------------------------------------------------------------------+
|                     NATIVE LINKING & PACKAGING                           |
|   Windows (EXE / MSIX / DLLs)  <------>  Linux (ELF / AppImage / Flatpak)|
|   Android (APK/AAB via NDK)    <------>  macOS/iOS (.app / .ipa / Xcode) |
|   Web (WASM via Emscripten)                                              |
+--------------------------------------------------------------------------+
```

---

# 2. Host Toolchain Management & Target Triples

For Phase 1 V1.0, PingDart prioritizes **native host builds** (building Windows targets natively on Windows, and Linux targets natively on Linux) to guarantee toolchain stability before expanding into full cross-compilation matrixes.

### 2.1 Supported Compiler Matrix

| Target OS | Primary Compiler | Minimum Version | Alternative Compiler | Build Generator |
|---|---|---|---|---|
| **Windows** | MSVC (Visual Studio 2022 v143) | 19.34+ (MSVC 17.4) | Clang-CL 16+ | Ninja / MSBuild |
| **Linux** | Clang (`clang++`) | 16.0+ | GCC 12.0+ (C++20 baseline) | Ninja |
| **macOS / iOS** (Future) | Apple Clang (Xcode 15+) | 15.0+ | LLVM Clang 16+ | Ninja / Xcode |
| **Android** (Future) | Clang (Android NDK r25+) | NDK r25c+ | N/A | Ninja |
| **Web** (Future) | Emscripten (`emcc`/`em++`) | 3.1.40+ | N/A | Ninja |

### 2.2 Target Configuration Schema & Triples

PingDart standardizes on LLVM-canonical target triples:
- Windows x64: `x86_64-pc-windows-msvc`
- Windows ARM64: `aarch64-pc-windows-msvc`
- Linux x64: `x86_64-unknown-linux-gnu`
- Linux ARM64: `aarch64-unknown-linux-gnu`

#### Project Target Schema (`pingdart.yaml`)
```yaml
name: counter_app
version: 1.0.0
entrypoint: src/main.cpp
sdk_version: ">=1.0.0 <2.0.0"

targets:
  windows:
    min_os_version: "10.0.17763"
    architectures: [x64, arm64]
    package_formats: [portable, msix]
    capabilities: [vulkan, d3d12]
  linux:
    min_glibc_version: "2.31"
    architectures: [x64, arm64]
    package_formats: [appimage, flatpak]
    backends: [wayland, x11]

dependencies:
  pingdart_ui: "1.0.0"
```

---

# 3. Dependency Resolution, Hermeticity & Reproducible Builds

PingDart strictly distinguishes between **Hermetic Builds** and **Reproducible Builds**:
- **Reproducibility:** Ensures that identical source inputs, toolchain versions, and configurations generate bit-identical binary hashes across different machines and dates (achieved via path remapping, deterministic timestamps, and fixed flags).
- **Hermeticity:** Ensures that the build process cannot access undeclared external host dependencies, unpinned system headers, or ambient environment configurations (achieved by isolating environment variables, pinning toolchains, and locking dependencies).

### 3.1 Lockfile Specification & Hermetic Constraints (`pingdart.lock`)
SHA-256 dependency locks are necessary but insufficient on their own. PingDart enforces a full hermetic contract:
1. **Toolchain & SDK Pinning:** `pingdart.lock` explicitly pins compiler toolchain versions (e.g. Clang 16.0.6, MSVC 19.34), Windows SDK builds (10.0.22621.0), and Linux sysroots.
2. **Environment Sanitization:** When `pd build` executes, it sanitizes ambient environment variables (`PATH`, `INCLUDE`, `LIB`, `CPATH`, `LIBRARY_PATH`), injecting only hermetically declared paths to prevent ambient host contamination.
3. **Cryptographic Checksums:** All prebuilt libraries (Skia, HarfBuzz, FreeType, SQLite, Vulkan headers) are validated against exact SHA-256 hashes:

```yaml
version: 1
toolchain:
  clang_version: "16.0.6"
  msvc_version: "19.34.31937"
  windows_sdk: "10.0.22621.0"
  cmake_version: "3.27.4"
  ninja_version: "1.11.1"

packages:
  skia:
    version: "m118-e3a1f8"
    url: "https://packages.pingdart.dev/deps/skia-m118-linux-x64.tar.gz"
    sha256: "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08"
  harfbuzz:
    version: "8.2.1"
    url: "https://packages.pingdart.dev/deps/harfbuzz-8.2.1.tar.gz"
    sha256: "5cb75efbb9ad2d8dfd50a26e85cbccba1c9d9d28dbd60cf4e0a779dd5843a0e1"
```

### 3.2 Build Graph Optimization & Caching
1. **Compilation Databases:** Every build generates a standardized `compile_commands.json` enabling IDE auto-complete and static analysis (Clangd, Clang-Tidy).
2. **Compiler Cache (ccache / sccache):** The `pd` orchestrator detects and activates `ccache` (on Linux) or `sccache` (on Windows/MSVC) automatically, cutting rebuild times by over 80%.
3. **Dependency Invalidation:** CMake tracks header dependencies via Ninja's native `.ninja_deps` header scanning. Only source files with modified includes or compilation flags are re-compiled.

---

# 4. PingDartCore Integration (Rust Compiler ↔ C++ Engine)

PingDartCore (the Rust compiler subsystem) integrates with the C++ framework via an explicit, version-negotiated C-ABI contract.

### 4.1 Artifact Exchange
- **Debug Builds:** Linked dynamically (`pingdart_core_compiler.dll` on Windows, `libpingdart_core_compiler.so` on Linux).
- **Release Builds:** Linked statically as a hermetic static library (`pingdart_core_compiler.lib` / `libpingdart_core_compiler.a`).

### 4.2 Version Negotiation & ABI Boundary (`pd_compiler_abi.h`)

```c
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PINGDART_COMPILER_ABI_VERSION 10001 // 1.0.1

typedef enum pd_status_t {
    PD_STATUS_OK = 0,
    PD_STATUS_ERR_INVALID_ARGUMENT = 1,
    PD_STATUS_ERR_PARSE_FAILURE = 2,
    PD_STATUS_ERR_INTERNAL_PANIC = 3,
    PD_STATUS_ERR_VERSION_MISMATCH = 4
} pd_status_t;

typedef struct pd_compiler_ctx_t pd_compiler_ctx_t;

// Queries Rust compiler ABI version
uint32_t pd_compiler_get_abi_version(void);

// Initializes Rust compiler instance
pd_status_t pd_compiler_create(pd_compiler_ctx_t** out_ctx);

// Destroys Rust compiler instance and reclaims memory
void pd_compiler_destroy(pd_compiler_ctx_t* ctx);

// Compiles PingDart source buffer into AST representation
pd_status_t pd_compiler_compile_source(
    pd_compiler_ctx_t* ctx,
    const char* source_code,
    size_t source_len,
    uint8_t** out_ast_bytes,
    size_t* out_ast_len
);

// Releases buffer allocated by the Rust runtime
void pd_compiler_free_buffer(uint8_t* buffer);

#ifdef __cplusplus
}
#endif
```

#### ABI Safety Contract:
- **Version Verification:** Upon startup, C++ queries `pd_compiler_get_abi_version()`. If it does not match `PINGDART_COMPILER_ABI_VERSION`, initialization aborts with `PD_STATUS_ERR_VERSION_MISMATCH`.
- **Memory Safety:** Buffers returned via `out_ast_bytes` are strictly released using `pd_compiler_free_buffer()`.

---

# 5. The Official Developer CLI (`pd`)

The `pd` binary is the developer's single entry point, orchestrating toolchains without exposing raw build files.

### 5.1 CLI Command Specification

```bash
# 1. Environment and toolchain verification
pd doctor

# 2. Scaffolding a new application project
pd init <app_name> [--template default|minimal]

# 3. Compiling native target
pd build <platform> [--arch x64|arm64] [--release|--debug]

# 4. Building and launching application on desktop
pd run <platform> [--release|--debug]

# 5. Running unit, widget, and golden tests
pd test [--filter <pattern>]

# 6. Packaging for distribution
pd package <platform> [--format msix|portable|appimage|flatpak]

# 7. Cleaning build artifacts
pd clean
```

### 5.2 `pd doctor` Diagnostics Flow
`pd doctor` runs automated diagnostic checks on developer workstations:
```text
[✓] C++ Compiler: Clang 16.0.6 detected (/usr/bin/clang++)
[✓] Build Generator: Ninja 1.11.1 detected (/usr/bin/ninja)
[✓] Configuration Engine: CMake 3.27.4 detected (/usr/bin/cmake)
[✓] Graphics Driver: Vulkan SDK 1.3.261.1 found (VK_LAYER_KHRONOS_validation available)
[✓] Linux Display: Wayland Compositor (GNOME) connected via WAYLAND_DISPLAY
[!] Optional: Android NDK not found (set ANDROID_NDK_ROOT to build for Android)
• Diagnostics passed: Machine ready to build Windows/Linux native targets.
```

---

# 6. Desktop Packaging Architecture

### 6.1 Windows Packaging Pipelines

1. **Self-Contained Portable Packaging (EXE & Assemblies):**
   - *Technical Prerequisite:* A raw Windows `.exe` is **not automatically self-contained**. Runtime dependencies are packaged according to explicit deployment rules:
     - **VC++ Redistributable:** Always bundles the Microsoft Visual C++ runtime DLLs (`msvcp140.dll`, `vcruntime140.dll`, `vcruntime140_1.dll`) in the application directory.
     - **Direct3D 12 Agility SDK:** Bundled conditionally in a `D3D12/` subfolder only when the application requires advanced D3D12 feature levels (e.g. Shader Model 6.6+, Enhanced Barriers) on older Windows 10 versions that lack native OS driver support. For standard desktop builds, the native system `d3d12.dll` is preferred.
     - **Vulkan Loader:** The Vulkan loader (`vulkan-1.dll` / `libvulkan.so.1`) is typically provided by the host GPU driver / operating system. Bundling a local loader is conditional and restricted to standalone offline enterprise distributions.
     - **License Notices:** Bundles `THIRD_PARTY_LICENSES.txt`, fulfilling legal attribution obligations for Skia, HarfBuzz, FreeType, and SQLite.
2. **MSIX Installer Packaging:**
   - Generates an `AppxManifest.xml` defining application capabilities, display name, tile icons, and identity.
   - Executes `MakeAppx.exe pack /d <staging_dir> /p <output.msix>`.
   - Digitally signs the package using `SignTool.exe sign /fd SHA256 /f <cert.pfx> <output.msix>`.

### 6.2 Linux Packaging Pipelines

1. **AppImage Pipeline:**
   - Constructs standard AppDir hierarchy:
     ```text
     MyApp.AppDir/
     ├── AppRun (executable launcher script)
     ├── myapp (compiled native ELF binary)
     ├── myapp.desktop (XDG desktop metadata)
     ├── myapp.png (application icon)
     └── usr/lib/ (bundled dependencies, e.g. libxkbcommon)
     ```
   - Executes `appimagetool MyApp.AppDir output.AppImage`.
2. **Flatpak Pipeline:**
   - Generates `org.pingdart.myapp.yaml` manifest specifying runtime (`org.freedesktop.Platform 23.08`).
   - Runs `flatpak-builder --force-clean build-dir org.pingdart.myapp.yaml`.

---

# 7. Mobile & Web Toolchain Preparation

While Phase 1 targets desktop native builds, the build system reserves clear hooks for mobile and web toolchains:

### 7.1 Android Preparation (Future Phase)
- The application pipeline provides a standalone CMake toolchain file (`android.toolchain.cmake`).
- `pd build android` invokes a lightweight Gradle wrapper driving the Android NDK, linking `libpingdart_engine.so` into the final APK/AAB bundle.

### 7.2 Apple Preparation (macOS & iOS Future Phase)
- Generates Xcode project files via CMake (`cmake -G Xcode`).
- Automates bundle codesigning and notarization via `xcrun notarytool`.

### 7.3 Web Preparation (WebAssembly Future Phase)
- Injects Emscripten SDK environment (`source emsdk_env.sh`).
- Compiles application source into WebAssembly bytecode (`.wasm`) with `-sUSE_WEBGPU=1` and `-sALLOW_MEMORY_GROWTH=1`.

---

# 8. Security, Code Signing & Build Provenance

1. **Dependency Integrity Verification:**
   - Before executing a build, `pd` validates all downloaded prebuilt binaries and dependencies against the SHA-256 hashes recorded in `pingdart.lock`. Any hash mismatch aborts the build with `SecurityViolationException`.
2. **Deterministic & Reproducible Builds:**
   - Compiles with path remapping:
     - Clang/GCC: `-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.`
     - MSVC: `/experimental:deterministic` and `/Brepro`
   - Zero machine-specific filesystem paths are leaked into release binaries.
3. **Automated Signing:**
   - Supports hardware security module (HSM) tokens and cloud key management (Azure Key Vault / AWS KMS) for enterprise build signing.

---

# 9. CI/CD Matrix Specification

PingDart uses automated native matrix pipelines across Windows and Linux:

```yaml
# .github/workflows/native_build_matrix.yml
name: Native Host CI Matrix

on: [push, pull_request]

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: windows-2022
            platform: windows
            compiler: msvc
            arch: x64
          - os: ubuntu-22.04
            platform: linux
            compiler: clang
            arch: x64

    runs-on: ${{ matrix.os }}

    steps:
      - name: Checkout Code
        uses: actions/checkout@v4

      - name: Install Linux Dependencies
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y ninja-build libwayland-dev libxkbcommon-dev libx11-dev libvulkan-dev

      - name: Run pd doctor
        run: pd doctor

      - name: Native Build & Test
        run: |
          pd build ${{ matrix.platform }} --release
          pd test

      - name: Package Desktop Artifacts
        run: pd package ${{ matrix.platform }}
```

---

# 10. Diagnostics & Structured Error Formatting

When compilation or layout errors occur, `pd` formats errors with source context, line numbers, and actionable suggestions:

```text
[ERROR] Compilation failed in src/screens/home_screen.cpp:24:18

   22 |     Row({
   23 |         Text("User Settings"),
   24 |         Expanded(Button("Save"))
      |                  ^^^^^^^^^^^^^^
   25 |     })

Error: RenderFlex children have non-zero flex, but incoming constraints are unbounded along the main axis.
Suggestion: Wrap the Row in a SizedBox with explicit width, or replace Expanded with Flexible(fit: FlexFit::Loose).
```

---

# 11. DOC-04 Acceptance Milestone

The build system architecture is verified when:
1. `pd init test_app` scaffolds a complete, compilable project.
2. `pd doctor` passes on both Windows 11 and Ubuntu 22.04.
3. `pd build windows` and `pd build linux` compile native binaries using shared application C++ source code.
4. `pd test` runs unit and layout tests cleanly.
5. `pd package` produces a working Windows portable/MSIX package and a Linux AppImage.

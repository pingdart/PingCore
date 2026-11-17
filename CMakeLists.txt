cmake_minimum_required(VERSION 3.26)

project(PingDart
    VERSION 0.1.0
    DESCRIPTION "PingDart Cross-Platform Native Application Framework"
    LANGUAGES C CXX
)

# Load compiler options and CTest
include(cmake/CompilerOptions.cmake)
include(CTest)

# Resolve dependencies (from pingdart.lock)
include(cmake/Dependencies.cmake)

# Rust PingDart Compiler FFI Subsystem
set(RUST_COMPILER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust/pingdart_compiler")
set(RUST_LIB_STATIC "${RUST_COMPILER_DIR}/target/release/${CMAKE_STATIC_LIBRARY_PREFIX}pingdart_compiler${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_custom_command(
    OUTPUT ${RUST_LIB_STATIC}
    COMMAND cargo build --release
    WORKING_DIRECTORY ${RUST_COMPILER_DIR}
    DEPENDS
        ${RUST_COMPILER_DIR}/src/lib.rs
        ${RUST_COMPILER_DIR}/Cargo.toml
    COMMENT "Building Rust PingDart compiler FFI library"
)

add_custom_target(pingdart_compiler_cargo DEPENDS ${RUST_LIB_STATIC})

add_library(pingdart_compiler STATIC IMPORTED GLOBAL)
set_target_properties(pingdart_compiler PROPERTIES
    IMPORTED_LOCATION "${RUST_LIB_STATIC}"
)
add_dependencies(pingdart_compiler pingdart_compiler_cargo)

# Core Framework Static Library
add_library(pingdart_core STATIC
    src/core/runtime.cpp
    src/core/event_loop.cpp
    src/core/compiler.cpp
)

target_include_directories(pingdart_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pingdart_core
    PUBLIC
        Threads::Threads
        pingdart_compiler
)

add_library(PingDart::Core ALIAS pingdart_core)

# Platform Abstraction Layer (PAL) Interface Library
add_library(pingdart_pal INTERFACE)
target_include_directories(pingdart_pal
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(pingdart_pal
    INTERFACE
        PingDart::Core
)
add_library(PingDart::PAL ALIAS pingdart_pal)

# Mobile Platform Abstraction Scaffolding (MOBILE-101: Android NDK & iOS UIKit/Metal)
add_library(pingdart_pal_mobile STATIC
    src/pal/android/android_platform.cpp
    src/pal/ios/ios_platform.cpp
)
target_include_directories(pingdart_pal_mobile
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(pingdart_pal_mobile
    PUBLIC
        PingDart::Core
        PingDart::PAL
)
add_library(PingDart::PALMobile ALIAS pingdart_pal_mobile)

# Platform Adapters (Conditional inclusion)
if(WIN32)
    add_subdirectory(src/pal/windows)
elseif(UNIX AND NOT APPLE)
    add_subdirectory(src/pal/linux)
endif()

# Rendering Hardware Interface (RHI)
add_subdirectory(src/rhi)

# Text Rendering & Typography Engine (HarfBuzz, Unicode, Font Manager)
if(Freetype_FOUND AND HarfBuzz_FOUND)
    add_subdirectory(src/text)
    add_subdirectory(src/ui)
endif()

# First-Class Web Platform & SEO Engine
add_subdirectory(src/web)

# Native Packaging & Toolchain CLI (ENG-601)
add_subdirectory(src/cli)

# Package Manager & Ecosystem Registry (PKG-101..PKG-107)
add_subdirectory(src/pkg)
add_subdirectory(tools/pd)

# Examples
if(Freetype_FOUND AND HarfBuzz_FOUND)
    add_subdirectory(examples/desktop_host)
    add_subdirectory(examples/desktop_login)
endif()
add_subdirectory(examples/web_hello)

# Testing
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

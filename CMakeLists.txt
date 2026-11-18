cmake_minimum_required(VERSION 3.26)

project(PingDart
    VERSION 0.1.0
    DESCRIPTION "PingDart Cross-Platform Native Application Framework"
    LANGUAGES C CXX
)

# Load compiler options and CTest
include(cmake/CompilerOptions.cmake)
include(CTest)

# Security & Sanitizer Toolchains (PKG-115)
include(cmake/Sanitizers.cmake)
include(cmake/SecurityHardening.cmake)
include(cmake/StaticAnalysis.cmake)

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
if(WIN32)
    set_target_properties(pingdart_compiler PROPERTIES
        INTERFACE_LINK_LIBRARIES "ws2_32;userenv;ntdll;bcrypt"
    )
endif()
add_dependencies(pingdart_compiler pingdart_compiler_cargo)

# Core Framework Static Library
add_library(pingdart_core STATIC
    src/core/runtime.cpp
    src/core/event_loop.cpp
    src/core/compiler.cpp
    src/pal/native_bridge.cpp
    src/pal/android/native_activity_main.cpp
)

target_include_directories(pingdart_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pingdart_core
    PUBLIC
        Threads::Threads
        $<BUILD_INTERFACE:pingdart_compiler>
    PRIVATE
        $<BUILD_INTERFACE:PingDart::Sanitizers>
        $<BUILD_INTERFACE:PingDart::SecurityHardening>
)

add_library(PingDart::Core ALIAS pingdart_core)

# Safety & Security Subsystems (PKG-115)
add_subdirectory(src/safety)
add_subdirectory(src/security)

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
    src/pal/android/android_platform_host.cpp
    src/pal/android/android_window.cpp
    src/pal/android/android_input_dispatcher.cpp
    src/pal/ios/ios_platform_host.mm
    src/pal/ios/ios_window.mm
    src/pal/ios/ios_input_dispatcher.mm
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
elseif(APPLE AND NOT IOS)
    add_subdirectory(src/pal/macos)
elseif(UNIX AND NOT APPLE)
    add_subdirectory(src/pal/linux)
endif()

# Rendering Hardware Interface (RHI)
add_subdirectory(src/rhi)

# Text Rendering & Typography Engine (HarfBuzz, Unicode, Font Manager)
add_subdirectory(src/text)

# Declarative Widget Engine & Layout System (ENG-501..ENG-503)
add_subdirectory(src/ui)

# First-Class Web Platform & SEO Engine
add_subdirectory(src/web)

# Native Packaging & Toolchain CLI (ENG-601)
add_subdirectory(src/cli)

# Package Manager & Ecosystem Registry (PKG-101..PKG-107)
add_subdirectory(src/pkg)
add_subdirectory(tools/pd)

# Backend API & Server Engine (PKG-119)
add_subdirectory(src/server)
target_link_libraries(pd PRIVATE pingcore_server)

# Modular Target Architecture & Aliases (PKG-119)
add_library(PingCore::Core ALIAS pingdart_core)
add_library(PingCore::Security ALIAS pingdart_security)
add_library(PingCore::Database ALIAS pingdart_pkg)

if(TARGET pingdart_ui)
    add_library(PingCore::UI ALIAS pingdart_ui)
endif()

add_library(pingcore_network INTERFACE)
target_include_directories(pingcore_network INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(pingcore_network INTERFACE
    PingCore::Core
    PingCore::Server
)
add_library(PingCore::Network ALIAS pingcore_network)

# SDK Umbrella Target
add_library(pingcore_sdk INTERFACE)
set_target_properties(pingcore_sdk PROPERTIES EXPORT_NAME SDK)
target_include_directories(pingcore_sdk INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(pingcore_sdk INTERFACE
    PingCore::Core
    PingCore::Server
    PingCore::Security
    PingCore::Database
    PingDart::PAL
    PingDart::RHI
)
if(TARGET pingdart_ui)
    target_link_libraries(pingcore_sdk INTERFACE PingCore::UI PingDart::Text)
endif()
if(APPLE AND NOT IOS)
    target_link_libraries(pingcore_sdk INTERFACE pingdart_pal_macos)
endif()
add_library(PingCore::SDK ALIAS pingcore_sdk)

# --- Standalone SDK Installation & CMake Export (PKG-119) ---
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set_target_properties(pingdart_core PROPERTIES EXPORT_NAME Core)
set_target_properties(pingcore_server PROPERTIES EXPORT_NAME Server)
set_target_properties(pingdart_security PROPERTIES EXPORT_NAME Security)
set_target_properties(pingdart_safety PROPERTIES EXPORT_NAME Safety)
set_target_properties(pingdart_pkg PROPERTIES EXPORT_NAME Database)
set_target_properties(pingdart_pal PROPERTIES EXPORT_NAME PAL)
set_target_properties(pingdart_rhi PROPERTIES EXPORT_NAME RHI)
set_target_properties(pingdart_sanitizers PROPERTIES EXPORT_NAME Sanitizers)
set_target_properties(pingdart_security_hardening PROPERTIES EXPORT_NAME SecurityHardening)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS
    pingdart_core
    pingcore_server
    pingdart_security
    pingdart_safety
    pingdart_pkg
    pingdart_pal
    pingdart_rhi
    pingdart_sanitizers
    pingdart_security_hardening
    EXPORT PingCoreTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(TARGETS pingcore_sdk
    EXPORT PingCoreTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES "${RUST_LIB_STATIC}" DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(TARGET pingdart_text)
    set_target_properties(pingdart_text PROPERTIES EXPORT_NAME Text)
    install(TARGETS pingdart_text EXPORT PingCoreTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if(TARGET pingdart_pal_macos)
    install(TARGETS pingdart_pal_macos EXPORT PingCoreTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if(TARGET pingdart_ui)
    set_target_properties(pingdart_ui PROPERTIES EXPORT_NAME UI)
    install(TARGETS pingdart_ui EXPORT PingCoreTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

install(TARGETS pd RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT PingCoreTargets
    FILE PingCoreTargets.cmake
    NAMESPACE PingCore::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PingCore
)
export(EXPORT PingCoreTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/PingCoreTargets.cmake"
    NAMESPACE PingCore::
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/PingCoreConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

get_property(PINGCORE_CONFIG_HAS_FREETYPE GLOBAL PROPERTY PINGCORE_HAS_FREETYPE)
get_property(PINGCORE_CONFIG_HAS_HARFBUZZ GLOBAL PROPERTY PINGCORE_HAS_HARFBUZZ)
get_property(PINGCORE_CONFIG_HAS_OPENSSL GLOBAL PROPERTY PINGCORE_HAS_OPENSSL)
foreach(dependency Freetype HarfBuzz OpenSSL)
    if(NOT PINGCORE_CONFIG_HAS_${dependency})
        set(PINGCORE_CONFIG_HAS_${dependency} FALSE)
    endif()
endforeach()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindHarfBuzz.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FindHarfBuzz.cmake"
    COPYONLY
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PingCoreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PingCoreConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PingCore
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/PingCoreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/PingCoreConfigVersion.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/FindHarfBuzz.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PingCore
)

# Examples
set(PINGDART_EXAMPLES
    examples/desktop_host
    examples/desktop_login
    examples/shared_login
    examples/my_first_app
    examples/web_hello
    examples/doctor_card
    examples/component_gallery
    examples/reactive_counter
    examples/login_form
    examples/doctor_profile
    examples/responsive_dashboard
    examples/interactive_datagrid
    examples/gesture_playground
)
foreach(example_dir IN LISTS PINGDART_EXAMPLES)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${example_dir}/CMakeLists.txt")
        add_subdirectory("${example_dir}")
    endif()
endforeach()

# Testing
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

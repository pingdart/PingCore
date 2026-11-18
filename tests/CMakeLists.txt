# tests/CMakeLists.txt

# Standalone assert-based smoke test
add_executable(test_core_smoke
    core/test_smoke.cpp
)
target_link_libraries(test_core_smoke PRIVATE
    PingDart::Core
)
add_test(NAME CoreSmokeTest COMMAND test_core_smoke)

# GoogleTest-based unit test suite (verified via ENG-102)
if(TARGET gtest_main)
    add_executable(test_runtime_gtest
        core/test_runtime_gtest.cpp
    )
    target_link_libraries(test_runtime_gtest PRIVATE
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME RuntimeGoogleTest COMMAND test_runtime_gtest)
    add_executable(test_result
        core/test_result.cpp
    )
    target_link_libraries(test_result PRIVATE
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME ResultGoogleTest COMMAND test_result)

    add_executable(test_ref_ptr
        core/test_ref_ptr.cpp
    )
    target_link_libraries(test_ref_ptr PRIVATE
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME RefPtrGoogleTest COMMAND test_ref_ptr)

    add_executable(test_event_loop
        core/test_event_loop.cpp
    )
    target_link_libraries(test_event_loop PRIVATE
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME EventLoopGoogleTest COMMAND test_event_loop)

    add_executable(test_compiler_ffi
        core/test_compiler_ffi.cpp
    )
    target_link_libraries(test_compiler_ffi PRIVATE
        PingDart::Core
        pingdart_compiler
        GTest::gtest_main
    )
    add_test(NAME CompilerFfiGoogleTest COMMAND test_compiler_ffi)

    add_executable(test_pal_contracts
        pal/test_pal_contracts.cpp
    )
    target_link_libraries(test_pal_contracts PRIVATE
        PingDart::PAL
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME PalContractsGoogleTest COMMAND test_pal_contracts)

    add_executable(test_wayland_protocol
        pal/test_wayland_protocol.cpp
        ../src/pal/linux/wayland/wayland_input_dispatcher.cpp
    )
    target_include_directories(test_wayland_protocol PRIVATE
        ${CMAKE_SOURCE_DIR}/src/pal/linux/wayland
    )
    target_link_libraries(test_wayland_protocol PRIVATE
        PingDart::PAL
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME WaylandProtocolGoogleTest COMMAND test_wayland_protocol)

    add_executable(test_x11_protocol
        pal/test_x11_protocol.cpp
        ../src/pal/linux/x11/x11_input_dispatcher.cpp
    )
    target_include_directories(test_x11_protocol PRIVATE
        ${CMAKE_SOURCE_DIR}/src/pal/linux/x11
    )
    target_link_libraries(test_x11_protocol PRIVATE
        PingDart::PAL
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME X11ProtocolGoogleTest COMMAND test_x11_protocol)

    add_executable(test_unified_input
        pal/test_unified_input.cpp
    )
    target_link_libraries(test_unified_input PRIVATE
        PingDart::PAL
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME UnifiedInputGoogleTest COMMAND test_unified_input)

    # macOS Platform Tests (PKG-113: Cocoa NSWindow & CAMetalLayer)
    if(APPLE AND NOT IOS)
        add_executable(test_macos_platform
            pal/test_macos_platform.mm
        )
        target_link_libraries(test_macos_platform PRIVATE
            PingDart::PALMacos
            PingDart::PAL
            PingDart::Core
            GTest::gtest_main
            "-framework Cocoa"
            "-framework Metal"
            "-framework QuartzCore"
            "-framework Carbon"
        )
        target_compile_options(test_macos_platform PRIVATE -fobjc-arc)
        add_test(NAME MacosPlatformGoogleTest COMMAND test_macos_platform)
    endif()

    # Mobile Platform Tests (MOBILE-101: Android NDK & iOS UIKit/Metal)
    add_executable(test_mobile_platform
        pal/test_mobile_platform.cpp
    )
    target_link_libraries(test_mobile_platform PRIVATE
        PingDart::PAL
        PingDart::PALMobile
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME MobilePlatformGoogleTest COMMAND test_mobile_platform)

    add_executable(test_rhi_contracts
        rhi/test_rhi_contracts.cpp
    )
    target_link_libraries(test_rhi_contracts PRIVATE
        PingDart::RHI
        PingDart::PAL
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME RhiContractsGoogleTest COMMAND test_rhi_contracts)

    add_executable(test_rhi_lifecycle
        rhi/test_rhi_lifecycle.cpp
    )
    target_link_libraries(test_rhi_lifecycle PRIVATE
        PingDart::RHI
        PingDart::PAL
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME RhiLifecycleGoogleTest COMMAND test_rhi_lifecycle)

    add_executable(test_vulkan_swapchain
        rhi/test_vulkan_swapchain.cpp
    )
    target_link_libraries(test_vulkan_swapchain PRIVATE
        PingDart::RHI
        PingDart::PAL
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME VulkanSwapchainGoogleTest COMMAND test_vulkan_swapchain)

    add_executable(test_vulkan_pipeline
        rhi/test_vulkan_pipeline.cpp
    )
    target_link_libraries(test_vulkan_pipeline PRIVATE
        PingDart::RHI
        PingDart::PAL
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME VulkanPipelineGoogleTest COMMAND test_vulkan_pipeline)

    # Native Metal Backend Tests (PKG-114: Apple Silicon / Metal RHI)
    if(APPLE)
        add_executable(test_metal_backend
            rhi/test_metal_backend.mm
        )
        target_link_libraries(test_metal_backend PRIVATE
            PingDart::RHI
            PingDart::PALMacos
            PingDart::PAL
            PingDart::Core
            GTest::gtest_main
            "-framework Metal"
            "-framework QuartzCore"
            "-framework Cocoa"
        )
        target_compile_options(test_metal_backend PRIVATE -fobjc-arc)
        add_test(NAME MetalBackendGoogleTest COMMAND test_metal_backend)
    endif()

    add_executable(test_display_list
        rhi/test_display_list.cpp
    )
    target_link_libraries(test_display_list PRIVATE
        PingDart::RHI
        PingDart::PAL
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME DisplayListGoogleTest COMMAND test_display_list)

    add_executable(test_text_shaping
        text/test_text_shaping.cpp
    )
    target_link_libraries(test_text_shaping PRIVATE
        PingDart::Text
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME TextShapingGoogleTest COMMAND test_text_shaping)

    add_executable(test_paragraph_layout
        text/test_paragraph_layout.cpp
    )
    target_link_libraries(test_paragraph_layout PRIVATE
        PingDart::Text
        PingDart::RHI
        PingDart::PAL
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME ParagraphLayoutGoogleTest COMMAND test_paragraph_layout)

    add_executable(test_web_seo
        web/test_web_seo.cpp
    )
    target_link_libraries(test_web_seo PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME WebSeoGoogleTest COMMAND test_web_seo)

    add_executable(test_browser_scheduler
        web/test_browser_scheduler.cpp
    )
    target_link_libraries(test_browser_scheduler PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME BrowserSchedulerGoogleTest COMMAND test_browser_scheduler)

    add_executable(test_wasm_runtime
        web/test_wasm_runtime.cpp
    )
    target_link_libraries(test_wasm_runtime PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME WasmRuntimeGoogleTest COMMAND test_wasm_runtime)

    add_executable(test_semantic_widgets
        web/test_semantic_widgets.cpp
    )
    target_link_libraries(test_semantic_widgets PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME SemanticWidgetsGoogleTest COMMAND test_semantic_widgets)

    add_executable(test_json_ld_security
        web/test_json_ld_security.cpp
    )
    target_link_libraries(test_json_ld_security PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME JsonLdSecurityGoogleTest COMMAND test_json_ld_security)

    add_executable(test_dom_reconciliation
        web/test_dom_reconciliation.cpp
    )
    target_link_libraries(test_dom_reconciliation PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME DomReconciliationGoogleTest COMMAND test_dom_reconciliation)

    add_executable(test_dom_events
        web/test_dom_events.cpp
    )
    target_link_libraries(test_dom_events PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME DomEventsGoogleTest COMMAND test_dom_events)

    add_executable(test_ssr_server
        web/test_ssr_server.cpp
    )
    target_link_libraries(test_ssr_server PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME SsrServerGoogleTest COMMAND test_ssr_server)

    add_executable(test_static_generator
        web/test_static_generator.cpp
    )
    target_link_libraries(test_static_generator PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME StaticGeneratorGoogleTest COMMAND test_static_generator)

    add_executable(test_route_enumerator
        web/test_route_enumerator.cpp
    )
    target_link_libraries(test_route_enumerator PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME RouteEnumeratorGoogleTest COMMAND test_route_enumerator)

    add_executable(test_ssg_security
        web/test_ssg_security.cpp
    )
    target_link_libraries(test_ssg_security PRIVATE
        PingDart::Web
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME SsgSecurityGoogleTest COMMAND test_ssg_security)

    # Declarative Widget Engine & Layout System Tests (ENG-501..ENG-503)
    add_executable(test_widget_tree
        ui/test_widget_tree.cpp
    )
    target_link_libraries(test_widget_tree PRIVATE
        PingDart::UI
        PingDart::RHI
        PingDart::Text
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME WidgetTreeGoogleTest COMMAND test_widget_tree)

    add_executable(test_reconciliation
        ui/test_reconciliation.cpp
    )
    target_link_libraries(test_reconciliation PRIVATE
        PingDart::UI
        PingDart::RHI
        PingDart::Text
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME ReconciliationGoogleTest COMMAND test_reconciliation)

    add_executable(test_layout_constraints
        ui/test_layout_constraints.cpp
    )
    target_link_libraries(test_layout_constraints PRIVATE
        PingDart::UI
        PingDart::RHI
        PingDart::Text
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME LayoutConstraintsGoogleTest COMMAND test_layout_constraints)

    # INPUT-101..106: Native Input, Focus, Graphemes, IME & Caret Tests
    add_executable(test_native_input
        ui/test_native_input.cpp
    )
    target_link_libraries(test_native_input PRIVATE
        PingDart::UI
        PingDart::PAL
        PingDart::RHI
        PingDart::Text
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME NativeInputGoogleTest COMMAND test_native_input)

    # TEXT-104: FreeType, HarfBuzz, Glyph Atlas & Text Renderer Tests
    add_executable(test_freetype
        text/test_freetype.cpp
    )
    if(Freetype_FOUND)
        target_compile_definitions(test_freetype PRIVATE PINGDART_HAS_FREETYPE=1)
        target_include_directories(test_freetype PRIVATE ${FREETYPE_INCLUDE_DIRS})
        target_link_libraries(test_freetype PRIVATE Freetype::Freetype)
    endif()
    target_link_libraries(test_freetype PRIVATE
        PingDart::Text
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME FreeTypeGoogleTest COMMAND test_freetype)

    add_executable(test_harfbuzz
        text/test_harfbuzz.cpp
    )
    if(HarfBuzz_FOUND)
        target_compile_definitions(test_harfbuzz PRIVATE PINGDART_HAS_HARFBUZZ=1)
        target_include_directories(test_harfbuzz PRIVATE ${HarfBuzz_INCLUDE_DIR})
        target_link_libraries(test_harfbuzz PRIVATE HarfBuzz::HarfBuzz)
    endif()
    target_link_libraries(test_harfbuzz PRIVATE
        PingDart::Text
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME HarfBuzzGoogleTest COMMAND test_harfbuzz)

    add_executable(test_glyph_atlas
        text/test_glyph_atlas.cpp
    )
    target_link_libraries(test_glyph_atlas PRIVATE
        PingDart::Text
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME GlyphAtlasGoogleTest COMMAND test_glyph_atlas)

    add_executable(test_glyph_cache
        text/test_glyph_cache.cpp
    )
    target_link_libraries(test_glyph_cache PRIVATE
        PingDart::Text
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME GlyphCacheGoogleTest COMMAND test_glyph_cache)

    add_executable(test_text_renderer
        text/test_text_renderer.cpp
    )
    if(HarfBuzz_FOUND)
        target_compile_definitions(test_text_renderer PRIVATE PINGDART_HAS_HARFBUZZ=1)
        target_link_libraries(test_text_renderer PRIVATE HarfBuzz::HarfBuzz)
    endif()
    target_link_libraries(test_text_renderer PRIVATE
        PingDart::Text
        PingDart::Core
        PingDart::RHI
        GTest::gtest_main
    )
    add_test(NAME TextRendererGoogleTest COMMAND test_text_renderer)

    # Native Packaging Suite (ENG-601)
    add_executable(test_packaging
        cli/test_packaging.cpp
    )
    target_link_libraries(test_packaging PRIVATE
        PingDart::CLI
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME PackagingGoogleTest COMMAND test_packaging)

    # Package Manager Suite (PKG-101..PKG-107)
    add_executable(test_pkg_manager
        pkg/test_pkg_manager.cpp
    )
    target_link_libraries(test_pkg_manager PRIVATE
        PingDart::PKG
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME PackageManagerGoogleTest COMMAND test_pkg_manager)

    # PKG-108: Public Registry API and Developer Portal Tests
    add_executable(test_pkg_registry_api
        pkg/test_pkg_registry_api.cpp
    )
    target_link_libraries(test_pkg_registry_api PRIVATE
        PingDart::PKG
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME PkgRegistryApiGoogleTest COMMAND test_pkg_registry_api)

    # PKG-109: Package Scanning, Advisories and Publisher Security Tests
    add_executable(test_pkg_security_scanner
        pkg/test_pkg_security_scanner.cpp
    )
    target_link_libraries(test_pkg_security_scanner PRIVATE
        PingDart::PKG
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME PkgSecurityScannerGoogleTest COMMAND test_pkg_security_scanner)

    # PKG-110: Private Registries, Scoped Packages & Isolated Sandbox Tests
    add_executable(test_pkg_scoped_registries
        pkg/test_pkg_scoped_registries.cpp
    )
    target_link_libraries(test_pkg_scoped_registries PRIVATE
        PingDart::PKG
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME PkgScopedRegistriesGoogleTest COMMAND test_pkg_scoped_registries)

    # PKG-111: Production Security & Staging Deployment Suite
    add_executable(test_pkg_production_security
        pkg/test_pkg_production_security.cpp
    )
    target_link_libraries(test_pkg_production_security PRIVATE
        PingDart::PKG
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME PkgProductionSecurityGoogleTest COMMAND test_pkg_production_security)

    # PKG-112: Staging Registry End-to-End Publish, Download, Install & Recovery Suite
    add_executable(test_pkg_staging_e2e
        pkg/test_pkg_staging_e2e.cpp
    )
    target_link_libraries(test_pkg_staging_e2e PRIVATE
        PingDart::PKG
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME PkgStagingE2EGoogleTest COMMAND test_pkg_staging_e2e)

    # PKG-115: Memory Safety Primitives Suite
    add_executable(test_safe_primitives
        safety/test_safe_primitives.cpp
    )
    target_link_libraries(test_safe_primitives PRIVATE
        PingDart::Safety
        PingDart::Core
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    add_test(NAME SafePrimitivesGoogleTest COMMAND test_safe_primitives)

    # PKG-115: Security Subsystems & OS Sandbox Isolation Suite
    add_executable(test_security_hardening
        security/test_security_hardening.cpp
    )
    target_link_libraries(test_security_hardening PRIVATE
        PingDart::Security
        PingDart::Safety
        PingDart::PKG
        PingDart::Core
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    add_test(NAME SecurityHardeningGoogleTest COMMAND test_security_hardening)

    # PKG-116: Fuzz Testing Suite for Parsers, Manifests, Archives, and Boundaries
    add_executable(test_security_fuzzing
        fuzz/test_security_fuzzing.cpp
    )
    target_link_libraries(test_security_fuzzing PRIVATE
        PingDart::PKG
        PingDart::Security
        PingDart::Safety
        PingDart::Core
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    add_test(NAME SecurityFuzzingGoogleTest COMMAND test_security_fuzzing)

    # PKG-117: Mobile Platform Lifecycle, Multi-Touch & Packaging Suites
    add_executable(test_mobile_lifecycle
        mobile/test_mobile_lifecycle.cpp
    )
    target_link_libraries(test_mobile_lifecycle PRIVATE
        PingDart::PALMobile
        PingDart::Core
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    add_test(NAME MobileLifecycleGoogleTest COMMAND test_mobile_lifecycle)

    add_executable(test_mobile_touch
        mobile/test_mobile_touch.cpp
    )
    target_link_libraries(test_mobile_touch PRIVATE
        shared_login_lib
        PingDart::PALMobile
        PingDart::Core
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    add_test(NAME MobileTouchGoogleTest COMMAND test_mobile_touch)

    add_executable(test_mobile_packaging
        mobile/test_mobile_packaging.cpp
    )
    target_link_libraries(test_mobile_packaging PRIVATE
        shared_login_lib
        PingDart::PALMobile
        PingDart::Core
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    add_test(NAME MobilePackagingGoogleTest COMMAND test_mobile_packaging)

    # PKG-118: PingCore Developer SDK Tests
    add_executable(test_pingcore_sdk
        sdk/test_pingcore_sdk.cpp
    )
    target_include_directories(test_pingcore_sdk PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pingcore_sdk PRIVATE
        PingDart::Core
        PingDart::PAL
        PingDart::RHI
        PingDart::UI
        PingDart::Text
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pingcore_sdk PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME PingCoreSdkGoogleTest COMMAND test_pingcore_sdk)

    # PKG-119: PingCore Backend API SDK Tests
    add_executable(test_pingcore_server
        server/test_pingcore_server.cpp
    )
    target_include_directories(test_pingcore_server PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pingcore_server PRIVATE
        pingcore_server
        PingDart::Core
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    add_test(NAME PingCoreServerGoogleTest COMMAND test_pingcore_server)

    # PKG-119: Visual Correctness & Golden Image Verification Tests
    add_executable(test_golden_image
        visual/test_golden_image.cpp
    )
    target_include_directories(test_golden_image PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_golden_image PRIVATE
        PingDart::Core
        PingDart::PAL
        PingDart::RHI
        PingDart::UI
        PingDart::Text
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_golden_image PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME GoldenImageGoogleTest COMMAND test_golden_image)

    # PKG-119: Next.js-Style File-Based Router & UI Components Tests
    add_executable(test_nextjs_router
        router/test_nextjs_router.cpp
    )
    target_include_directories(test_nextjs_router PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_nextjs_router PRIVATE
        pingcore_server
        PingDart::Core
        PingDart::PAL
        PingDart::RHI
        PingDart::UI
        PingDart::Text
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_nextjs_router PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME NextJsRouterGoogleTest COMMAND test_nextjs_router)

    # PKG-120: Web Runtime, SSR, SEO, Route Compiler & Security Tests
    add_executable(test_pkg120_web_ssr
        web/test_pkg120_web_ssr.cpp
    )
    target_include_directories(test_pkg120_web_ssr PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg120_web_ssr PRIVATE
        pingcore_server
        PingDart::Web
        PingDart::Core
        PingDart::PAL
        PingDart::RHI
        PingDart::UI
        PingDart::Text
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg120_web_ssr PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg120WebSsrGoogleTest COMMAND test_pkg120_web_ssr)

    # PKG-121: Real Native Platforms Scaffolding, Permissions & Native Bridge Tests
    add_executable(test_pkg121_native_platforms
        native/test_pkg121_native_platforms.cpp
    )
    target_include_directories(test_pkg121_native_platforms PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg121_native_platforms PRIVATE
        PingDart::Core
        PingDart::PAL
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg121_native_platforms PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg121NativePlatformsGoogleTest COMMAND test_pkg121_native_platforms)

    # PKG-122: Native Module System, Async C ABI & Version-Aware Permissions Tests
    add_executable(test_pkg122_native_modules
        native/test_pkg122_native_modules.cpp
    )
    target_include_directories(test_pkg122_native_modules PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg122_native_modules PRIVATE
        PingDart::Core
        PingDart::PAL
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg122_native_modules PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg122NativeModulesGoogleTest COMMAND test_pkg122_native_modules)

    # PKG-123: Native Module Runtime Integration, Async State Machine, Mutex Discipline & Atomic Removal Tests
    add_executable(test_pkg123_runtime_integration
        native/test_pkg123_runtime_integration.cpp
    )
    target_include_directories(test_pkg123_runtime_integration PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg123_runtime_integration PRIVATE
        PingDart::Core
        PingDart::PAL
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg123_runtime_integration PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg123RuntimeIntegrationGoogleTest COMMAND test_pkg123_runtime_integration)

    # PKG-124: Full-Stack Reactivity Core, Signals, Queries, Hydration, Async Server & Hot Reload Tests
    add_executable(test_pkg124_fullstack_runtime
        native/test_pkg124_fullstack_runtime.cpp
    )
    target_include_directories(test_pkg124_fullstack_runtime PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg124_fullstack_runtime PRIVATE
        PingDart::Core
        PingDart::PAL
        PingDart::UI
        PingDart::RHI
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg124_fullstack_runtime PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg124FullStackRuntimeGoogleTest COMMAND test_pkg124_fullstack_runtime)

    # PKG-125: Production Runtime Validation, Performance & Reliability Tests
    add_executable(test_pkg125_production_validation
        native/test_pkg125_production_validation.cpp
    )
    target_include_directories(test_pkg125_production_validation PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg125_production_validation PRIVATE
        pingcore_server
        PingDart::Core
        PingDart::PAL
        PingDart::UI
        PingDart::RHI
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg125_production_validation PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg125ProductionValidationGoogleTest COMMAND test_pkg125_production_validation)

    # PKG-126: Production Hardening & Real-World End-to-End Certification Tests
    add_executable(test_pkg126_production_hardening
        native/test_pkg126_production_hardening.cpp
    )
    target_include_directories(test_pkg126_production_hardening PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg126_production_hardening PRIVATE
        pingcore_server
        PingDart::Core
        PingDart::PAL
        PingDart::UI
        PingDart::RHI
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg126_production_hardening PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg126ProductionHardeningGoogleTest COMMAND test_pkg126_production_hardening)

    # PKG-127: Real Browser, TCP/TLS & Native Device Certification Tests
    add_executable(test_pkg127_browser_tcp_certification
        native/test_pkg127_browser_tcp_certification.cpp
    )
    target_include_directories(test_pkg127_browser_tcp_certification PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg127_browser_tcp_certification PRIVATE
        pingcore_server
        PingDart::Core
        PingDart::PAL
        PingDart::UI
        PingDart::RHI
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg127_browser_tcp_certification PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg127BrowserTcpCertificationGoogleTest COMMAND test_pkg127_browser_tcp_certification)

    # PKG-128: End-to-End Full-Stack Application Certification Tests
    add_executable(test_pkg128_fullstack_certification
        native/test_pkg128_fullstack_certification.cpp
    )
    target_include_directories(test_pkg128_fullstack_certification PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg128_fullstack_certification PRIVATE
        pingcore_server
        PingDart::Core
        PingDart::PAL
        PingDart::UI
        PingDart::RHI
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg128_fullstack_certification PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg128FullStackCertificationGoogleTest COMMAND test_pkg128_fullstack_certification)

    # PKG-129: Real Application Runtime & Deployment Certification Tests
    add_executable(test_pkg129_runtime_deployment
        native/test_pkg129_runtime_deployment.cpp
    )
    target_include_directories(test_pkg129_runtime_deployment PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg129_runtime_deployment PRIVATE
        pingcore_server
        PingDart::Core
        PingDart::PAL
        PingDart::UI
        PingDart::RHI
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg129_runtime_deployment PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg129RuntimeDeploymentGoogleTest COMMAND test_pkg129_runtime_deployment)

    # PKG-130: Genuine Native Runtime & Network Integration Certification Tests
    add_executable(test_pkg130_native_runtime_integration
        native/test_pkg130_native_runtime_integration.cpp
    )
    target_include_directories(test_pkg130_native_runtime_integration PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg130_native_runtime_integration PRIVATE
        pingcore_server
        PingDart::Core
        PingDart::PAL
        PingDart::UI
        PingDart::RHI
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg130_native_runtime_integration PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg130NativeRuntimeIntegrationGoogleTest COMMAND test_pkg130_native_runtime_integration)

    # PKG-132: Native Runtime Execution & Device Certification Tests
    add_executable(test_pkg132_native_runtime_execution
        native/test_pkg132_native_runtime_execution.cpp
    )
    target_include_directories(test_pkg132_native_runtime_execution PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg132_native_runtime_execution PRIVATE
        pingcore_server
        PingDart::Core
        PingDart::PAL
        PingDart::UI
        PingDart::RHI
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg132_native_runtime_execution PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg132NativeRuntimeExecutionGoogleTest COMMAND test_pkg132_native_runtime_execution)

    # PKG-133: Real Device End-to-End Acceptance Tests
    add_executable(test_pkg133_real_device_acceptance
        native/test_pkg133_real_device_acceptance.cpp
    )
    target_include_directories(test_pkg133_real_device_acceptance PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg133_real_device_acceptance PRIVATE
        pingcore_server
        PingDart::Core
        PingDart::PAL
        PingDart::UI
        PingDart::RHI
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    if(APPLE AND NOT IOS)
        target_link_libraries(test_pkg133_real_device_acceptance PRIVATE pingdart_pal_macos)
    endif()
    add_test(NAME Pkg133RealDeviceAcceptanceGoogleTest COMMAND test_pkg133_real_device_acceptance)

    # PKG-135: PingDartUI Foundation Tests
    add_executable(test_pingdart_ui_foundation
        ui/test_pingdart_ui_foundation.cpp
    )
    target_include_directories(test_pingdart_ui_foundation PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pingdart_ui_foundation PRIVATE
        PingDart::Core
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    add_test(NAME Pkg135PingDartUiFoundationGoogleTest COMMAND test_pingdart_ui_foundation)

    # PKG-136: PingDartUI Unified CSS & Utility Styling Engine Tests
    add_executable(test_pkg136_css_styling_engine
        ui/test_pkg136_css_styling_engine.cpp
    )
    target_include_directories(test_pkg136_css_styling_engine PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(test_pkg136_css_styling_engine PRIVATE
        PingDart::Core
        PingDart::Sanitizers
        PingDart::SecurityHardening
        GTest::gtest_main
    )
    add_test(NAME Pkg136CssStylingEngineGoogleTest COMMAND test_pkg136_css_styling_engine)
endif()

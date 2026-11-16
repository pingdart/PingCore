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
    target_link_libraries(test_freetype PRIVATE
        PingDart::Text
        PingDart::Core
        GTest::gtest_main
    )
    add_test(NAME FreeTypeGoogleTest COMMAND test_freetype)

    add_executable(test_harfbuzz
        text/test_harfbuzz.cpp
    )
    target_link_libraries(test_harfbuzz PRIVATE
        PingDart::Text
        PingDart::Core
        HarfBuzz::HarfBuzz
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
    target_link_libraries(test_text_renderer PRIVATE
        PingDart::Text
        PingDart::Core
        PingDart::RHI
        HarfBuzz::HarfBuzz
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
endif()

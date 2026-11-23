pingcore_add_test_suite(pingcore_windows_tests PingCoreWindowsTests
        LABELS platform windows native
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        SOURCES "${PINGCORE_TEST_DIR}/support/builtin_resources.cpp"
)
target_sources(pingcore_windows_tests PRIVATE
        "${PINGCORE_PROJECT_DIR}/examples/platform_module/color_stream.cpp"
        "${PINGCORE_PROJECT_DIR}/examples/platform_module/timer.cpp"
        "${PINGCORE_PROJECT_DIR}/examples/platform_module/windows_color_stream.cpp"
        "${PINGCORE_PROJECT_DIR}/examples/platform_module/windows_timer.cpp"
        accessibility.cpp
        application.cpp
        damage.cpp
        external_texture.cpp
        file.cpp
        http.cpp
        platform_module.cpp
        platform_view.cpp
        renderer.cpp
        window_chrome.cpp
)
target_include_directories(pingcore_windows_tests PRIVATE
        "${PINGCORE_PROJECT_DIR}/examples/platform_module"
)
target_link_libraries(pingcore_windows_tests PRIVATE ws2_32)
target_compile_definitions(pingcore_windows_tests PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)
target_include_directories(pingcore_windows_tests PRIVATE "${PINGCORE_PROJECT_DIR}/platform/windows")
target_compile_definitions(pingcore_windows_tests PRIVATE
        PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE="${PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE}"
)

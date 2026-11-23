pingcore_add_test_suite(pingcore_linux_tests PingCoreLinuxTests
        LABELS platform linux native
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        MAIN "${CMAKE_CURRENT_SOURCE_DIR}/tests_main.cpp"
        SOURCES "${PINGCORE_TEST_DIR}/support/builtin_resources.cpp"
)
target_sources(pingcore_linux_tests PRIVATE
        "${PINGCORE_PROJECT_DIR}/examples/platform_module/linux_timer.cpp"
        "${PINGCORE_PROJECT_DIR}/examples/platform_module/timer.cpp"
        external_texture.cpp
        file.cpp
        file_picker.cpp
        http.cpp
        platform_module.cpp
        platform_view.cpp
        renderer.cpp
        system_tray.cpp
        text_input.cpp
        window_chrome.cpp
)
target_include_directories(pingcore_linux_tests PRIVATE
        "${PINGCORE_PROJECT_DIR}/examples/platform_module"
        "$<TARGET_PROPERTY:pingcore_core_objects,INCLUDE_DIRECTORIES>"
)
target_link_libraries(pingcore_linux_tests PRIVATE
        PkgConfig::PINGCORE_GTK4
        PkgConfig::PINGCORE_GIO
)
target_include_directories(pingcore_linux_tests PRIVATE "${PINGCORE_PROJECT_DIR}/platform/linux")
target_compile_definitions(pingcore_linux_tests PRIVATE
        PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE="${PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE}"
)

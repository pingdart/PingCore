pingcore_add_test_suite(pingcore_macos_tests PingCoreMacOSTests
        LABELS platform macos native
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        SOURCES "${PINGCORE_TEST_DIR}/support/builtin_resources.cpp"
)
# App-scoped bookmark tests need a signature even on Intel, where the linker can leave executables unsigned.
target_link_options(pingcore_macos_tests PRIVATE "LINKER:-adhoc_codesign")
target_sources(pingcore_macos_tests PRIVATE
        "${PINGCORE_PROJECT_DIR}/examples/platform_module/timer.cpp"
        "${PINGCORE_PROJECT_DIR}/examples/platform_module/apple_timer.mm"
        local_notification.mm
        http.mm
        application.mm
        platform_module.mm
        platform_bridge.mm
        platform_view.mm
        external_texture.mm
        file_picker.mm
        text_input.mm
        window_chrome.cpp
)
set_source_files_properties(
        "${PINGCORE_PROJECT_DIR}/examples/platform_module/apple_timer.mm"
        platform_bridge.mm
        PROPERTIES COMPILE_OPTIONS "-fobjc-arc"
)
target_include_directories(pingcore_macos_tests PRIVATE
        "${PINGCORE_PROJECT_DIR}/examples/platform_module"
        "${PINGCORE_PROJECT_DIR}/platform/macos"
)
if (TARGET ${PINGCORE_STATIC_LIB_NAME})
    add_test(NAME PingCoreMacStaticPlatformBridgeImport
            COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/check_platform_bridge.sh"
                    "${PINGCORE_PROJECT_DIR}"
                    "${PINGCORE_PUBLIC_INCLUDE_DIR}"
                    "$<TARGET_FILE:${PINGCORE_STATIC_LIB_NAME}>"
                    "${CMAKE_CURRENT_BINARY_DIR}/macos_platform_bridge_static"
    )
endif ()
if (TARGET ${PINGCORE_SHARED_LIB_NAME})
    add_test(NAME PingCoreMacSharedPlatformBridgeImport
            COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/check_platform_bridge.sh"
                    "${PINGCORE_PROJECT_DIR}"
                    "${PINGCORE_PUBLIC_INCLUDE_DIR}"
                    "$<TARGET_FILE:${PINGCORE_SHARED_LIB_NAME}>"
                    "${CMAKE_CURRENT_BINARY_DIR}/macos_platform_bridge_shared"
    )
endif ()
target_compile_definitions(pingcore_macos_tests PRIVATE
        PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE="${PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE}"
)
foreach (bridge_test IN ITEMS PingCoreMacStaticPlatformBridgeImport PingCoreMacSharedPlatformBridgeImport)
    if (TEST ${bridge_test})
        set_tests_properties(${bridge_test} PROPERTIES LABELS "platform;macos;native;headers")
    endif ()
endforeach ()

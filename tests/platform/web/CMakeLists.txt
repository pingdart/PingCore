pingcore_add_test_suite(pingcore_web_logic_tests PingCoreWebLogicTests
        LABELS platform web portable
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        SOURCES text.cpp
)
target_include_directories(pingcore_web_logic_tests PRIVATE "${PINGCORE_PROJECT_DIR}/platform/web")

if (NODE_JS_EXECUTABLE)
    set(PINGCORE_WEB_TEST_COMMAND "${NODE_JS_EXECUTABLE}")
elseif (EMSCRIPTEN AND CMAKE_CROSSCOMPILING_EMULATOR)
    set(PINGCORE_WEB_TEST_COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR})
else ()
    find_program(PINGCORE_NODE_COMMAND NAMES node nodejs)
    set(PINGCORE_WEB_TEST_COMMAND "${PINGCORE_NODE_COMMAND}")
endif ()
if (PINGCORE_WEB_TEST_COMMAND)
    add_test(NAME PingCoreWebPlatformRegistryTests
            COMMAND ${PINGCORE_WEB_TEST_COMMAND}
                    "${CMAKE_CURRENT_SOURCE_DIR}/platform_registry.js"
                    "${PINGCORE_PROJECT_DIR}/platform/web/web_platform_registry.js"
    )
    add_test(NAME PingCoreWebFilePickerTests
            COMMAND ${PINGCORE_WEB_TEST_COMMAND}
                    "${CMAKE_CURRENT_SOURCE_DIR}/file_picker.js"
                    "${PINGCORE_PROJECT_DIR}/platform/web/web_file_picker.cpp"
    )
    add_test(NAME PingCoreWebFontTests
            COMMAND ${PINGCORE_WEB_TEST_COMMAND}
                    "${CMAKE_CURRENT_SOURCE_DIR}/font.js"
                    "${PINGCORE_PROJECT_DIR}/platform/web/web_renderer.cpp"
    )
    set_tests_properties(PingCoreWebPlatformRegistryTests PingCoreWebFilePickerTests PingCoreWebFontTests
            PROPERTIES LABELS "platform;web;portable"
    )
else ()
    message(STATUS "PingCore Web JavaScript tests are unavailable: Node.js was not found")
endif ()

if (EMSCRIPTEN)
    include(native.cmake)
endif ()

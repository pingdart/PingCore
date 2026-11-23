pingcore_add_test_suite(pingcore_web_tests PingCoreWebTests
        LABELS platform web native
        LIBRARIES ${PINGCORE_TEST_LIBRARY}
        SOURCES "${PINGCORE_TEST_DIR}/support/builtin_resources.cpp"
)
target_sources(pingcore_web_tests PRIVATE
        external_texture.cpp
        test_entry.cpp
        text_input.cpp
        text_layout.cpp
)
target_include_directories(pingcore_web_tests PRIVATE "${PINGCORE_PROJECT_DIR}/platform/web")
target_compile_definitions(pingcore_web_tests PRIVATE
        PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE="${PINGCORE_TEST_BUILTIN_RESOURCE_PACKAGE}"
)

add_test(NAME PingCoreWebTestFailureExitTests
        COMMAND "${CMAKE_COMMAND}"
                "-DTEST_EXECUTABLE=$<TARGET_FILE:pingcore_web_tests>"
                "-DTEST_EMULATOR=$<TARGET_PROPERTY:pingcore_web_tests,CROSSCOMPILING_EMULATOR>"
                -P "${CMAKE_CURRENT_LIST_DIR}/test_entry.cmake"
)
set_tests_properties(PingCoreWebTestFailureExitTests PROPERTIES
        LABELS "build;platform;web"
)

find_program(PINGCORE_WEB_TEST_BROWSER
        NAMES chromium chromium-browser google-chrome google-chrome-stable chrome "Google Chrome"
        PATHS "/Applications/Google Chrome.app/Contents/MacOS"
)
if (PINGCORE_WEB_TEST_BROWSER AND PINGCORE_WEB_TEST_COMMAND)
    pingcore_add_app(pingcore_web_file_storage_test SOURCES file_storage.cpp)
    set_target_properties(pingcore_web_file_storage_test PROPERTIES SUFFIX ".js")
    add_test(NAME PingCoreWebFileStorageTests
            COMMAND ${PINGCORE_WEB_TEST_COMMAND}
                    "${CMAKE_CURRENT_LIST_DIR}/file_storage_test.mjs"
                    "${PINGCORE_WEB_TEST_BROWSER}"
                    "$<TARGET_FILE:pingcore_web_file_storage_test>"
    )
    set_tests_properties(PingCoreWebFileStorageTests PROPERTIES LABELS "platform;web;native" TIMEOUT 45)
endif ()

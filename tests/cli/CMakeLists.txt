if (NOT TARGET pingcore_cli_core)
    return()
endif ()
file(TO_CMAKE_PATH "${PINGCORE_PROJECT_DIR}" PINGCORE_TEST_SOURCE_DIRECTORY)
function(pingcore_add_cli_test_suite target_name test_name)
    pingcore_add_test_suite(${target_name} ${test_name}
            LIBRARIES pingcore_cli_core
            ${ARGN}
    )
    target_include_directories(${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(${target_name} PRIVATE
            PINGCORE_TEST_SOURCE_DIRECTORY="${PINGCORE_TEST_SOURCE_DIRECTORY}"
    )
endfunction()
pingcore_add_cli_test_suite(pingcore_cli_tests PingCoreCliTests
        LABELS tools cli
        SOURCES commands.cpp project.cpp prompt.cpp sdk.cpp process.cpp
)
pingcore_add_cli_test_suite(pingcore_cli_windows_tests PingCoreCliWindowsTests
        LABELS tools cli platform windows portable
        SOURCES platform/windows.cpp
)
pingcore_add_cli_test_suite(pingcore_cli_linux_tests PingCoreCliLinuxTests
        LABELS tools cli platform linux portable
        SOURCES platform/linux.cpp
)
pingcore_add_cli_test_suite(pingcore_cli_macos_tests PingCoreCliMacOSTests
        LABELS tools cli platform macos portable
        SOURCES platform/macos.cpp
)
pingcore_add_cli_test_suite(pingcore_cli_android_tests PingCoreCliAndroidTests
        LABELS tools cli platform android portable
        SOURCES platform/android.cpp
)
pingcore_add_cli_test_suite(pingcore_cli_ios_tests PingCoreCliIosTests
        LABELS tools cli platform ios portable
        SOURCES platform/ios.cpp
)
pingcore_add_cli_test_suite(pingcore_cli_web_tests PingCoreCliWebTests
        LABELS tools cli platform web portable
        SOURCES platform/web.cpp
)

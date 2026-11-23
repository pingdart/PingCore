foreach (required_variable IN ITEMS
        BUILD_DIRECTORY BUILD_CONFIG WORK_DIRECTORY CPACK_COMMAND PACKAGE_FILE_NAME PACKAGE_GENERATOR
        PACKAGE_EXTENSION INSTALL_BINDIR INSTALL_LIBDIR CLI_SUFFIX HOST_PLATFORM HOST_ARCHITECTURE
)
    if (NOT DEFINED ${required_variable})
        message(FATAL_ERROR "${required_variable} is required")
    endif ()
endforeach ()

string(RANDOM LENGTH 12 ALPHABET 0123456789abcdef TEST_NONCE)
set(TEST_ROOT "${WORK_DIRECTORY}/hux-sdk-${TEST_NONCE}")
set(PACKAGE_DIRECTORY "${TEST_ROOT}/packages")
set(EXTRACT_DIRECTORY "${TEST_ROOT}/extract")
file(MAKE_DIRECTORY "${PACKAGE_DIRECTORY}" "${EXTRACT_DIRECTORY}")

set(PACKAGE_COMMAND
        "${CPACK_COMMAND}"
        --config "${BUILD_DIRECTORY}/CPackConfig.cmake"
        -G "${PACKAGE_GENERATOR}"
        -B "${PACKAGE_DIRECTORY}"
)
if (BUILD_CONFIG)
    list(APPEND PACKAGE_COMMAND -C "${BUILD_CONFIG}")
endif ()
execute_process(
        COMMAND ${PACKAGE_COMMAND}
        RESULT_VARIABLE PACKAGE_RESULT
        OUTPUT_VARIABLE PACKAGE_OUTPUT
        ERROR_VARIABLE PACKAGE_ERROR
)
if (NOT PACKAGE_RESULT EQUAL 0)
    message(FATAL_ERROR "SDK archive generation failed:\n${PACKAGE_OUTPUT}${PACKAGE_ERROR}")
endif ()

set(ARCHIVE_NAME "${PACKAGE_FILE_NAME}.${PACKAGE_EXTENSION}")
set(ARCHIVE_PATH "${PACKAGE_DIRECTORY}/${ARCHIVE_NAME}")
set(CHECKSUM_PATH "${ARCHIVE_PATH}.sha256")
if (NOT EXISTS "${ARCHIVE_PATH}")
    message(FATAL_ERROR "SDK archive is missing: ${ARCHIVE_PATH}")
endif ()
if (NOT EXISTS "${CHECKSUM_PATH}")
    message(FATAL_ERROR "SDK archive checksum is missing: ${CHECKSUM_PATH}")
endif ()

file(SHA256 "${ARCHIVE_PATH}" ARCHIVE_CHECKSUM)
file(READ "${CHECKSUM_PATH}" CHECKSUM_CONTENT)
string(FIND "${CHECKSUM_CONTENT}" "${ARCHIVE_CHECKSUM}" CHECKSUM_POSITION)
string(FIND "${CHECKSUM_CONTENT}" "${ARCHIVE_NAME}" CHECKSUM_NAME_POSITION)
if (CHECKSUM_POSITION LESS 0 OR CHECKSUM_NAME_POSITION LESS 0)
    message(FATAL_ERROR "SDK archive checksum does not match ${ARCHIVE_NAME}")
endif ()

file(ARCHIVE_EXTRACT INPUT "${ARCHIVE_PATH}" DESTINATION "${EXTRACT_DIRECTORY}")
set(SDK_ROOT "${EXTRACT_DIRECTORY}/${PACKAGE_FILE_NAME}")
file(REAL_PATH "${SDK_ROOT}" SDK_ROOT)
file(TO_NATIVE_PATH "${SDK_ROOT}" SDK_ROOT_NATIVE)
foreach (required_path IN ITEMS
        "LICENSE"
        "${INSTALL_BINDIR}/pingcore${CLI_SUFFIX}"
        "include/pingcore/data.h"
        "include/pingcore/pingcore.h"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreConfig.cmake"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreSharedTargets.cmake"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreStaticTargets.cmake"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreApp.cmake"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreCodegen.cmake"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreLibraries.cmake"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreOptimization.cmake"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreAndroid.exports"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreResourceBuild.cmake"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreResources.cmake"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreWebFileSystem.js"
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreWebPlatformRegistry.js"
        "share/pingcore/resources/pingcore/resources.bin"
        "share/pingcore/skills/pingcore-app-development/SKILL.md"
        "share/pingcore/skills/pingcore-app-development/references/project-workflow.md"
        "share/pingcore/skills/pingcore-app-development/references/resources-files-network.md"
        "share/pingcore/tools/${HOST_PLATFORM}/${HOST_ARCHITECTURE}/hcg${CLI_SUFFIX}"
        "share/pingcore/tools/${HOST_PLATFORM}/${HOST_ARCHITECTURE}/hrc${CLI_SUFFIX}"
)
    if (NOT EXISTS "${SDK_ROOT}/${required_path}")
        message(FATAL_ERROR "SDK archive is missing ${required_path}")
    endif ()
endforeach ()

if (EXISTS "${SDK_ROOT}/${INSTALL_LIBDIR}/cmake/PingCore/PingCoreTestingTargets.cmake")
    set(TESTING_REQUIRED_PATHS "include/pingcore/testing/ui_test.h")
    if (HOST_PLATFORM STREQUAL "windows" AND WINDOWS_DEBUG_INCLUDED)
        list(APPEND TESTING_REQUIRED_PATHS
                "${INSTALL_LIBDIR}/pingcore_testing.lib"
                "${INSTALL_LIBDIR}/pingcore_testing_debug.lib")
    endif ()
    if (PLATFORM_ARTIFACTS_INCLUDED)
        list(APPEND TESTING_REQUIRED_PATHS
                "share/pingcore/platform/android/arm64-v8a/libpingcore_testing.a"
                "share/pingcore/platform/android/x86_64/libpingcore_testing.a"
                "share/pingcore/platform/web/emscripten-4.0.19/libpingcore_testing.a")
        if (HOST_PLATFORM STREQUAL "macos")
            list(APPEND TESTING_REQUIRED_PATHS
                    "share/pingcore/platform/ios/PingCoreTesting.xcframework/ios-arm64/libpingcore_testing.a"
                    "share/pingcore/platform/ios/PingCoreTesting.xcframework/ios-arm64_x86_64-simulator/libpingcore_testing.a")
        endif ()
    endif ()
    foreach (required_path IN LISTS TESTING_REQUIRED_PATHS)
        if (NOT EXISTS "${SDK_ROOT}/${required_path}")
            message(FATAL_ERROR "SDK testing archive is missing ${required_path}")
        endif ()
    endforeach ()
endif ()

if (HOST_PLATFORM STREQUAL "windows" AND WINDOWS_DEBUG_INCLUDED)
    foreach (required_path IN ITEMS
            "bin/pingcore.dll"
            "bin/pingcore_debug.dll"
            "${INSTALL_LIBDIR}/pingcore.lib"
            "${INSTALL_LIBDIR}/pingcore_debug.lib"
            "${INSTALL_LIBDIR}/pingcore_static.lib"
            "${INSTALL_LIBDIR}/pingcore_static_debug.lib"
            "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreSharedTargets-release.cmake"
            "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreSharedTargets-debug.cmake"
            "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreStaticTargets-release.cmake"
            "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreStaticTargets-debug.cmake"
    )
        if (NOT EXISTS "${SDK_ROOT}/${required_path}")
            message(FATAL_ERROR "Windows SDK archive is missing ${required_path}")
        endif ()
    endforeach ()
endif ()

foreach (build_only_path IN ITEMS
        "${INSTALL_LIBDIR}/cmake/PingCore/PingCoreBuild.cmake"
        "${INSTALL_LIBDIR}/cmake/PingCore/platform"
)
    if (EXISTS "${SDK_ROOT}/${build_only_path}")
        message(FATAL_ERROR "SDK archive contains source-build CMake logic: ${build_only_path}")
    endif ()
endforeach ()

if (PLATFORM_ARTIFACTS_INCLUDED)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E tar tf
            "${SDK_ROOT}/share/pingcore/platform/android/PingCore.aar"
            RESULT_VARIABLE AAR_LIST_RESULT OUTPUT_VARIABLE AAR_ENTRIES ERROR_VARIABLE AAR_LIST_ERROR)
    if (NOT AAR_LIST_RESULT EQUAL 0)
        message(FATAL_ERROR "Cannot inspect SDK Android AAR: ${AAR_LIST_ERROR}")
    endif ()
    if (AAR_ENTRIES MATCHES "(^|\n)(jni|prefab)/")
        message(FATAL_ERROR "SDK Android AAR must be Java-only; CMake owns native dependencies")
    endif ()
    file(GLOB_RECURSE ANDROID_SMOKE_LIBRARIES "${SDK_ROOT}/share/pingcore/platform/android/*smoke*.so")
    if (ANDROID_SMOKE_LIBRARIES)
        message(FATAL_ERROR "SDK Android artifacts must not contain smoke libraries")
    endif ()
    foreach (required_path IN ITEMS
            "share/pingcore/platform/android/PingCore.aar"
            "share/pingcore/platform/android/arm64-v8a/libpingcore_static.a"
            "share/pingcore/platform/android/x86_64/libpingcore_static.a"
            "share/pingcore/platform/web/emscripten-4.0.19/libpingcore.a"
    )
        if (NOT EXISTS "${SDK_ROOT}/${required_path}")
            message(FATAL_ERROR "SDK archive is missing ${required_path}")
        endif ()
    endforeach ()

    foreach (unexpected_path IN ITEMS
            "share/pingcore/platform/android/arm64-v8a/libpingcore.so"
            "share/pingcore/platform/android/x86_64/libpingcore.so"
    )
        if (EXISTS "${SDK_ROOT}/${unexpected_path}")
            message(FATAL_ERROR "SDK archive contains an unexpected Android shared framework: ${unexpected_path}")
        endif ()
    endforeach ()

    set(TARGET_PLATFORM_CONSUMERS android web)
    if (HOST_PLATFORM STREQUAL "macos")
        foreach (required_path IN ITEMS
                "share/pingcore/platform/ios/PingCore.xcframework/Info.plist"
                "share/pingcore/platform/ios/PingCore.xcframework/ios-arm64/Headers/pingcore/data.h"
                "share/pingcore/platform/ios/PingCore.xcframework/ios-arm64/Headers/pingcore/pingcore.h"
                "share/pingcore/platform/ios/PingCore.xcframework/ios-arm64/libpingcore_static.a"
                "share/pingcore/platform/ios/PingCore.xcframework/ios-arm64_x86_64-simulator/Headers/pingcore/data.h"
                "share/pingcore/platform/ios/PingCore.xcframework/ios-arm64_x86_64-simulator/Headers/pingcore/pingcore.h"
                "share/pingcore/platform/ios/PingCore.xcframework/ios-arm64_x86_64-simulator/libpingcore_static.a"
        )
            if (NOT EXISTS "${SDK_ROOT}/${required_path}")
                message(FATAL_ERROR "macOS SDK archive is missing ${required_path}")
            endif ()
        endforeach ()
        list(APPEND TARGET_PLATFORM_CONSUMERS ios-device ios-simulator)
    elseif (EXISTS "${SDK_ROOT}/share/pingcore/platform/ios")
        message(FATAL_ERROR "Non-macOS SDK archive contains iOS artifacts")
    endif ()

    foreach (platform IN LISTS TARGET_PLATFORM_CONSUMERS)
        set(CONSUMER_SOURCE "${TEST_ROOT}/${platform}-consumer")
        set(CONSUMER_BUILD "${TEST_ROOT}/${platform}-consumer-build")
        file(MAKE_DIRECTORY "${CONSUMER_SOURCE}")
        if (platform STREQUAL "android")
            set(PLATFORM_CONFIGURATION "set(ANDROID TRUE)\nset(ANDROID_ABI arm64-v8a)")
            set(PLATFORM_ASSERTION
                    "if (NOT TARGET PingCore::pingcore_static)\n  message(FATAL_ERROR \"Android SDK is missing PingCore::pingcore_static\")\nendif ()"
            )
            set(EXPECTED_LIBRARY
                    "${SDK_ROOT}/share/pingcore/platform/android/arm64-v8a/libpingcore_static.a"
            )
        elseif (platform STREQUAL "web")
            set(PLATFORM_CONFIGURATION "set(EMSCRIPTEN TRUE)\nset(CMAKE_SIZEOF_VOID_P 4)")
            set(PLATFORM_ASSERTION)
            set(EXPECTED_LIBRARY
                    "${SDK_ROOT}/share/pingcore/platform/web/emscripten-4.0.19/libpingcore.a"
            )
        else ()
            if (platform STREQUAL "ios-simulator")
                set(IOS_SYSROOT iphonesimulator)
                set(IOS_SLICE ios-arm64_x86_64-simulator)
            else ()
                set(IOS_SYSROOT iphoneos)
                set(IOS_SLICE ios-arm64)
            endif ()
            set(PLATFORM_CONFIGURATION
                    "set(IOS TRUE)\nset(CMAKE_OSX_SYSROOT ${IOS_SYSROOT})"
            )
            set(PLATFORM_ASSERTION
                    "if (NOT TARGET PingCore::pingcore_static)\n  message(FATAL_ERROR \"iOS SDK does not expose PingCore::pingcore_static\")\nendif ()"
            )
            set(EXPECTED_LIBRARY
                    "${SDK_ROOT}/share/pingcore/platform/ios/PingCore.xcframework/${IOS_SLICE}/libpingcore_static.a"
            )
        endif ()
        file(WRITE "${CONSUMER_SOURCE}/CMakeLists.txt"
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(PingCoreTargetSdkConsumer LANGUAGES NONE)\n"
                "${PLATFORM_CONFIGURATION}\n"
                "find_package(PingCore CONFIG REQUIRED PATHS \"${SDK_ROOT}\" NO_DEFAULT_PATH)\n"
                "${PLATFORM_ASSERTION}\n"
                "get_target_property(location PingCore::pingcore IMPORTED_LOCATION)\n"
                "if (NOT location STREQUAL \"${EXPECTED_LIBRARY}\")\n"
                "  message(FATAL_ERROR \"Unexpected imported library: \${location}\")\n"
                "endif ()\n"
        )
        execute_process(
                COMMAND "${CMAKE_COMMAND}" -S "${CONSUMER_SOURCE}" -B "${CONSUMER_BUILD}"
                RESULT_VARIABLE CONSUMER_RESULT
                OUTPUT_VARIABLE CONSUMER_OUTPUT
                ERROR_VARIABLE CONSUMER_ERROR
        )
        if (NOT CONSUMER_RESULT EQUAL 0)
            message(FATAL_ERROR
                    "Installed ${platform} SDK import failed:\n${CONSUMER_OUTPUT}${CONSUMER_ERROR}"
            )
        endif ()
    endforeach ()
    unset(TARGET_PLATFORM_CONSUMERS)
endif ()

file(GLOB HOST_DIRECTORIES
        LIST_DIRECTORIES TRUE
        RELATIVE "${SDK_ROOT}/share/pingcore/tools"
        "${SDK_ROOT}/share/pingcore/tools/*"
)
if (NOT HOST_DIRECTORIES STREQUAL HOST_PLATFORM)
    message(FATAL_ERROR "SDK archive contains unexpected host tool directories: ${HOST_DIRECTORIES}")
endif ()
file(GLOB HOST_ARCHITECTURE_DIRECTORIES
        LIST_DIRECTORIES TRUE
        RELATIVE "${SDK_ROOT}/share/pingcore/tools/${HOST_PLATFORM}"
        "${SDK_ROOT}/share/pingcore/tools/${HOST_PLATFORM}/*"
)
if (NOT HOST_ARCHITECTURE_DIRECTORIES STREQUAL HOST_ARCHITECTURE)
    message(FATAL_ERROR
            "SDK archive contains unexpected host tool architectures: ${HOST_ARCHITECTURE_DIRECTORIES}"
    )
endif ()

execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env --unset=PINGCORE_HOME
                "${SDK_ROOT}/${INSTALL_BINDIR}/pingcore${CLI_SUFFIX}" doctor
        WORKING_DIRECTORY "${TEST_ROOT}"
        RESULT_VARIABLE DOCTOR_RESULT
        OUTPUT_VARIABLE DOCTOR_OUTPUT
        ERROR_VARIABLE DOCTOR_ERROR
)
string(FIND "${DOCTOR_OUTPUT}" "[ok] PINGCORE_HOME (executable): ${SDK_ROOT_NATIVE}" SDK_OUTPUT_POSITION)
if (NOT DOCTOR_RESULT EQUAL 0 OR SDK_OUTPUT_POSITION LESS 0 OR DOCTOR_ERROR)
    message(FATAL_ERROR "Archived SDK self-discovery failed:\n${DOCTOR_OUTPUT}${DOCTOR_ERROR}")
endif ()

file(REMOVE_RECURSE "${TEST_ROOT}")

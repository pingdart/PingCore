include_guard(GLOBAL)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(PINGCORE_WEB_EMSCRIPTEN_VERSION "4.0.19")

if (ANDROID)
    if (NOT ANDROID_ABI STREQUAL "arm64-v8a")
        message(FATAL_ERROR "PingCore Android host SDK supports arm64-v8a only")
    endif ()
    set(PINGCORE_SDK_HOST_PLATFORM "android")
    set(PINGCORE_SDK_HOST_ARCHITECTURE "${ANDROID_ABI}")
else ()
    _pingcore_resolve_host(PINGCORE_SDK_HOST_PLATFORM PINGCORE_SDK_HOST_ARCHITECTURE)
endif ()

if (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR
        AND PINGCORE_BUILD_CLI
        AND (WIN32 OR (APPLE AND NOT IOS) OR (UNIX AND NOT APPLE AND NOT EMSCRIPTEN)))
    set(PINGCORE_SDK_PACKAGE_FILE_NAME
            "pingcore-sdk-${PROJECT_VERSION}-${PINGCORE_SDK_HOST_PLATFORM}-${PINGCORE_SDK_HOST_ARCHITECTURE}"
    )
    if (WIN32)
        set(PINGCORE_SDK_PACKAGE_GENERATOR "ZIP")
        set(PINGCORE_SDK_PACKAGE_EXTENSION "zip")
    else ()
        set(PINGCORE_SDK_PACKAGE_GENERATOR "TGZ")
        set(PINGCORE_SDK_PACKAGE_EXTENSION "tar.gz")
    endif ()
endif ()

if (TARGET pingcore_testing AND NOT ANDROID)
    install(EXPORT PingCoreTestingTargets FILE PingCoreTestingTargets.cmake
            NAMESPACE PingCore:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/PingCore"
            COMPONENT PingCoreLibraries)
endif ()

install(DIRECTORY "${PINGCORE_PUBLIC_HEADER_DIR}"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
if (APPLE AND NOT IOS)
    install(FILES "${PINGCORE_PROJECT_DIR}/platform/macos/PingCorePlatform.modulemap"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            RENAME module.modulemap
    )
endif ()
install(DIRECTORY "${PINGCORE_BUILTIN_RESOURCE_PACKAGE}/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/pingcore/resources"
)
install(DIRECTORY "${PINGCORE_PROJECT_DIR}/skills/pingcore-app-development"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/pingcore/skills"
)
install(FILES "${PINGCORE_PROJECT_DIR}/LICENSE" DESTINATION ".")

set(PINGCORE_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/PingCore")
configure_package_config_file(
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/PingCoreConfig.cmake"
        INSTALL_DESTINATION "${PINGCORE_INSTALL_CMAKE_DIR}"
)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND TARGET ${PINGCORE_STATIC_LIB_NAME})
    configure_file(
            "${PINGCORE_PROJECT_DIR}/cmake/PingCoreLinuxStaticDependencies.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/PingCoreLinuxStaticDependencies.cmake"
            @ONLY
    )
endif ()
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/PingCoreConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
)

if (TARGET ${PINGCORE_SHARED_LIB_NAME}
        AND NOT PINGCORE_SDK_HOST_PLATFORM STREQUAL "android")
    install(EXPORT PingCoreSharedTargets
            FILE PingCoreSharedTargets.cmake
            NAMESPACE PingCore::
            DESTINATION "${PINGCORE_INSTALL_CMAKE_DIR}"
            COMPONENT PingCoreLibraries
    )
endif ()
if (TARGET ${PINGCORE_STATIC_LIB_NAME}
        AND NOT PINGCORE_SDK_HOST_PLATFORM STREQUAL "android")
    install(EXPORT PingCoreStaticTargets
            FILE PingCoreStaticTargets.cmake
            NAMESPACE PingCore::
            DESTINATION "${PINGCORE_INSTALL_CMAKE_DIR}"
            COMPONENT PingCoreLibraries
    )
endif ()
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/PingCoreConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/PingCoreConfigVersion.cmake"
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreApp.cmake"
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreCodegen.cmake"
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreLibraries.cmake"
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreOptimization.cmake"
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreAndroid.exports"
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreResourceBuild.cmake"
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreResources.cmake"
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreRuntimeDependencies.cmake"
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreGenerateWixPayloads.cmake"
        "${PINGCORE_PROJECT_DIR}/cmake/PingCoreWindowsInstaller.cmake"
        DESTINATION "${PINGCORE_INSTALL_CMAKE_DIR}"
)
if (WIN32)
    install(FILES
            "${PINGCORE_PROJECT_DIR}/platform/windows/win32_application_runner.h"
            "${PINGCORE_PROJECT_DIR}/platform/windows/windows_installer.cpp"
            DESTINATION "${PINGCORE_INSTALL_CMAKE_DIR}"
    )
endif ()
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND TARGET ${PINGCORE_STATIC_LIB_NAME})
    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/PingCoreLinuxStaticDependencies.cmake"
            DESTINATION "${PINGCORE_INSTALL_CMAKE_DIR}"
    )
endif ()
install(FILES "${PINGCORE_PROJECT_DIR}/platform/web/web_platform_registry.js"
        DESTINATION "${PINGCORE_INSTALL_CMAKE_DIR}"
        RENAME PingCoreWebPlatformRegistry.js
)
install(FILES "${PINGCORE_PROJECT_DIR}/platform/web/web_file.js"
        DESTINATION "${PINGCORE_INSTALL_CMAKE_DIR}"
        RENAME PingCoreWebFileSystem.js
)

if (TARGET pingcore_cli)
    install(TARGETS pingcore_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

if (PINGCORE_INTERNAL_SDK_ARTIFACT_ROOT)
    file(TO_CMAKE_PATH "${PINGCORE_INTERNAL_SDK_ARTIFACT_ROOT}" PINGCORE_SDK_ARTIFACT_ROOT)
    set(PINGCORE_ANDROID_ARTIFACT_ROOT "${PINGCORE_SDK_ARTIFACT_ROOT}/android")
    set(PINGCORE_WEB_ARTIFACT_ROOT
            "${PINGCORE_SDK_ARTIFACT_ROOT}/web/emscripten-${PINGCORE_WEB_EMSCRIPTEN_VERSION}"
    )
    foreach (PINGCORE_REQUIRED_PLATFORM_ARTIFACT IN ITEMS
            "${PINGCORE_ANDROID_ARTIFACT_ROOT}/PingCore.aar"
            "${PINGCORE_ANDROID_ARTIFACT_ROOT}/arm64-v8a/libpingcore_static.a"
            "${PINGCORE_ANDROID_ARTIFACT_ROOT}/x86_64/libpingcore_static.a"
            "${PINGCORE_WEB_ARTIFACT_ROOT}/libpingcore.a"
    )
        if (NOT EXISTS "${PINGCORE_REQUIRED_PLATFORM_ARTIFACT}")
            message(FATAL_ERROR "PingCore SDK platform artifact is missing: ${PINGCORE_REQUIRED_PLATFORM_ARTIFACT}")
        endif ()
    endforeach ()
    install(DIRECTORY "${PINGCORE_ANDROID_ARTIFACT_ROOT}/"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/pingcore/platform/android"
    )
    install(DIRECTORY "${PINGCORE_WEB_ARTIFACT_ROOT}/"
            DESTINATION
            "${CMAKE_INSTALL_DATADIR}/pingcore/platform/web/emscripten-${PINGCORE_WEB_EMSCRIPTEN_VERSION}"
    )
    if (APPLE AND NOT IOS)
        set(PINGCORE_IOS_XCFRAMEWORK
                "${PINGCORE_SDK_ARTIFACT_ROOT}/ios/PingCore.xcframework"
        )
        foreach (PINGCORE_REQUIRED_IOS_ARTIFACT IN ITEMS
                "${PINGCORE_IOS_XCFRAMEWORK}/Info.plist"
                "${PINGCORE_IOS_XCFRAMEWORK}/ios-arm64/Headers/module.modulemap"
                "${PINGCORE_IOS_XCFRAMEWORK}/ios-arm64/Headers/pingcore/data.h"
                "${PINGCORE_IOS_XCFRAMEWORK}/ios-arm64/Headers/pingcore/pingcore.h"
                "${PINGCORE_IOS_XCFRAMEWORK}/ios-arm64/libpingcore_static.a"
                "${PINGCORE_IOS_XCFRAMEWORK}/ios-arm64_x86_64-simulator/Headers/pingcore/data.h"
                "${PINGCORE_IOS_XCFRAMEWORK}/ios-arm64_x86_64-simulator/Headers/module.modulemap"
                "${PINGCORE_IOS_XCFRAMEWORK}/ios-arm64_x86_64-simulator/Headers/pingcore/pingcore.h"
                "${PINGCORE_IOS_XCFRAMEWORK}/ios-arm64_x86_64-simulator/libpingcore_static.a"
        )
            if (NOT EXISTS "${PINGCORE_REQUIRED_IOS_ARTIFACT}")
                message(FATAL_ERROR
                        "PingCore SDK iOS artifact is missing: ${PINGCORE_REQUIRED_IOS_ARTIFACT}"
                )
            endif ()
        endforeach ()
        install(DIRECTORY "${PINGCORE_IOS_XCFRAMEWORK}"
                DESTINATION "${CMAKE_INSTALL_DATADIR}/pingcore/platform/ios"
        )
        if (EXISTS "${PINGCORE_SDK_ARTIFACT_ROOT}/ios/PingCoreTesting.xcframework")
            install(DIRECTORY "${PINGCORE_SDK_ARTIFACT_ROOT}/ios/PingCoreTesting.xcframework"
                    DESTINATION "${CMAKE_INSTALL_DATADIR}/pingcore/platform/ios"
            )
        endif ()
    endif ()
endif ()

install(DIRECTORY
        "${PINGCORE_PROJECT_DIR}/tools/prebuilt/${PINGCORE_SDK_HOST_PLATFORM}/${PINGCORE_SDK_HOST_ARCHITECTURE}/"
        DESTINATION
        "${CMAKE_INSTALL_DATADIR}/pingcore/tools/${PINGCORE_SDK_HOST_PLATFORM}/${PINGCORE_SDK_HOST_ARCHITECTURE}"
        USE_SOURCE_PERMISSIONS
        PATTERN ".DS_Store" EXCLUDE
)

if (PINGCORE_SDK_PACKAGE_FILE_NAME)
    set(CPACK_PACKAGE_NAME "PingCore SDK")
    set(CPACK_PACKAGE_VENDOR "PingCore")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/pingdart/PingCore")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "PingCore cross-platform application development SDK")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "${PINGCORE_SDK_PACKAGE_FILE_NAME}")
    set(CPACK_PACKAGE_CHECKSUM "SHA256")
    set(CPACK_RESOURCE_FILE_LICENSE "${PINGCORE_PROJECT_DIR}/LICENSE")
    set(CPACK_GENERATOR "${PINGCORE_SDK_PACKAGE_GENERATOR}")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
    set(CPACK_MONOLITHIC_INSTALL ON)
    if (PINGCORE_SDK_HOST_PLATFORM STREQUAL "android")
        set(CPACK_STRIP_FILES "${CMAKE_INSTALL_BINDIR}/pingcore")
    endif ()
    if (WIN32 AND PINGCORE_INTERNAL_PACKAGE_WINDOWS_DEBUG)
        get_property(PINGCORE_SDK_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
        if (NOT PINGCORE_SDK_MULTI_CONFIG)
            message(FATAL_ERROR
                    "PingCore Windows SDK packaging with Debug libraries requires a multi-config generator")
        endif ()
        set(PINGCORE_WINDOWS_DEBUG_INSTALL_SCRIPT
                "${CMAKE_CURRENT_BINARY_DIR}/PingCoreInstallWindowsDebug.cmake"
        )
        file(WRITE "${PINGCORE_WINDOWS_DEBUG_INSTALL_SCRIPT}"
                "execute_process(\n"
                "  COMMAND \"${CMAKE_COMMAND}\" --install \"${PROJECT_BINARY_DIR}\" --config Debug"
                " --component PingCoreLibraries --prefix \"\${CMAKE_INSTALL_PREFIX}\"\n"
                "  RESULT_VARIABLE install_result\n"
                "  OUTPUT_VARIABLE install_output\n"
                "  ERROR_VARIABLE install_error\n"
                ")\n"
                "if (NOT install_result EQUAL 0)\n"
                "  message(FATAL_ERROR \"PingCore Windows Debug SDK installation failed:\\n"
                "\${install_output}\${install_error}\")\n"
                "endif ()\n"
        )
        set(CPACK_INSTALL_SCRIPTS "${PINGCORE_WINDOWS_DEBUG_INSTALL_SCRIPT}")
    endif ()
    include(CPack)
endif ()

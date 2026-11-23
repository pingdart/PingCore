add_test(NAME PingCoreProfilingBuildTests
        COMMAND "${CMAKE_COMMAND}"
                "-DLIBRARY_FILE=$<TARGET_FILE:${PINGCORE_TEST_LIBRARY}>"
                "-DTEST_EXECUTABLE=$<TARGET_FILE:pingcore_runtime_tests>"
                "-DEXPECT_PROFILING=$<BOOL:${PINGCORE_ENABLE_PROFILING}>"
                "-DCROSSCOMPILING=$<BOOL:${CMAKE_CROSSCOMPILING}>"
                "-DBUILD_DIRECTORY=${PROJECT_BINARY_DIR}"
                "-DBUILD_CONFIG=$<CONFIG>"
                "-DSDK_INSTALL_AVAILABLE=$<BOOL:${PINGCORE_SDK_HOST_PLATFORM}>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/profiling.cmake"
)
set_tests_properties(PingCoreProfilingBuildTests PROPERTIES LABELS "build")

if (CMAKE_CROSSCOMPILING)
    return()
endif ()

add_test(NAME PingCoreResourceMergeTests
        COMMAND "${CMAKE_COMMAND}"
                "-DSOURCE_DIRECTORY=${PINGCORE_PROJECT_DIR}"
                "-DWORK_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}"
                "-DHOST_GENERATOR=${CMAKE_GENERATOR}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/resource_merge.cmake"
)

add_test(NAME PingCoreLibraryValidationTests
        COMMAND "${CMAKE_COMMAND}"
                "-DSOURCE_DIRECTORY=${PINGCORE_PROJECT_DIR}"
                "-DWORK_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}"
                "-DHOST_GENERATOR=${CMAKE_GENERATOR}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/library_validation.cmake"
)

if (NOT CMAKE_CONFIGURATION_TYPES)
    add_test(NAME PingCoreAndroidAppIntegrationTests
            COMMAND "${CMAKE_COMMAND}"
                    "-DSOURCE_DIRECTORY=${PINGCORE_PROJECT_DIR}"
                    "-DWORK_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}"
                    "-DHOST_GENERATOR=${CMAKE_GENERATOR}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/android_app_integration.cmake"
    )
endif ()

find_program(PINGCORE_TEST_NINJA NAMES ninja)
if (PINGCORE_TEST_NINJA)
    add_test(NAME PingCoreAndroidStaticLinkTests
            COMMAND "${CMAKE_COMMAND}"
                    "-DSOURCE_DIRECTORY=${PINGCORE_PROJECT_DIR}"
                    "-DWORK_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}"
                    "-DHOST_NINJA=${PINGCORE_TEST_NINJA}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/android_static.cmake")
    set_tests_properties(PingCoreAndroidStaticLinkTests PROPERTIES
            LABELS "build;android" SKIP_REGULAR_EXPRESSION "PingCore Android NDK is unavailable")
endif ()

if (WIN32)
    set(PINGCORE_SOURCE_SUBDIRECTORY_WORK_DIRECTORY "$ENV{TEMP}")
else ()
    set(PINGCORE_SOURCE_SUBDIRECTORY_WORK_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif ()
if (WIN32 OR APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_test(NAME PingCoreRuntimeDependencyTests
            COMMAND "${CMAKE_COMMAND}"
                    "-DSOURCE_DIRECTORY=${PINGCORE_PROJECT_DIR}"
                    "-DWORK_DIRECTORY=${PINGCORE_SOURCE_SUBDIRECTORY_WORK_DIRECTORY}"
                    "-DHOST_GENERATOR=${CMAKE_GENERATOR}"
                    "-DHOST_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
                    "-DHOST_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
                    "-DHOST_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/runtime_dependencies.cmake"
    )
endif ()
add_test(NAME PingCoreSourceSubdirectoryTests
        COMMAND "${CMAKE_COMMAND}"
                "-DSOURCE_DIRECTORY=${PINGCORE_PROJECT_DIR}"
                "-DWORK_DIRECTORY=${PINGCORE_SOURCE_SUBDIRECTORY_WORK_DIRECTORY}"
                "-DHOST_GENERATOR=${CMAKE_GENERATOR}"
                "-DHOST_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
                "-DHOST_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
                "-DHOST_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/source_subdirectory.cmake"
)
if (MSVC AND CMAKE_GENERATOR MATCHES "^Ninja")
    set(PINGCORE_SOURCE_SUBDIRECTORY_ENVIRONMENT)
    foreach (PINGCORE_MSVC_ENVIRONMENT_VARIABLE IN ITEMS PATH INCLUDE LIB LIBPATH)
        string(REPLACE ";" "\\;" PINGCORE_MSVC_ENVIRONMENT_VALUE
                "$ENV{${PINGCORE_MSVC_ENVIRONMENT_VARIABLE}}"
        )
        list(APPEND PINGCORE_SOURCE_SUBDIRECTORY_ENVIRONMENT
                "${PINGCORE_MSVC_ENVIRONMENT_VARIABLE}=${PINGCORE_MSVC_ENVIRONMENT_VALUE}"
        )
    endforeach ()
    set_tests_properties(PingCoreSourceSubdirectoryTests PROPERTIES
            ENVIRONMENT "${PINGCORE_SOURCE_SUBDIRECTORY_ENVIRONMENT}"
    )
    if (TEST PingCoreRuntimeDependencyTests)
        set_tests_properties(PingCoreRuntimeDependencyTests PROPERTIES
                ENVIRONMENT "${PINGCORE_SOURCE_SUBDIRECTORY_ENVIRONMENT}"
        )
    endif ()
endif ()

if (TARGET pingcore_cli_core)
    add_test(NAME PingCoreProjectTemplateTests
            COMMAND "${CMAKE_COMMAND}"
                    "-DCLI_EXECUTABLE=$<TARGET_FILE:pingcore_cli>"
                    "-DSOURCE_DIRECTORY=${PINGCORE_PROJECT_DIR}"
                    "-DWORK_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}"
                    "-DHOST_GENERATOR=${CMAKE_GENERATOR}"
                    "-DHOST_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/project_templates.cmake"
    )
    if (MSVC AND CMAKE_GENERATOR MATCHES "^Ninja")
        set_tests_properties(PingCoreProjectTemplateTests PROPERTIES
                ENVIRONMENT "${PINGCORE_SOURCE_SUBDIRECTORY_ENVIRONMENT}"
        )
    endif ()
    if (WIN32)
        set(PINGCORE_GENERATED_LIBRARY_PLATFORM windows)
    elseif (APPLE)
        set(PINGCORE_GENERATED_LIBRARY_PLATFORM macos)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(PINGCORE_GENERATED_LIBRARY_PLATFORM linux)
    else ()
        unset(PINGCORE_GENERATED_LIBRARY_PLATFORM)
    endif ()
    if (TARGET pingcore_cli AND PINGCORE_GENERATED_LIBRARY_PLATFORM)
        add_test(NAME PingCoreGeneratedLibraryTests
                COMMAND "${CMAKE_COMMAND}"
                        "-DCLI_EXECUTABLE=$<TARGET_FILE:pingcore_cli>"
                        "-DSOURCE_DIRECTORY=${PINGCORE_PROJECT_DIR}"
                        "-DWORK_DIRECTORY=${PINGCORE_SOURCE_SUBDIRECTORY_WORK_DIRECTORY}"
                        "-DBUILD_CONFIG=$<CONFIG>"
                        "-DHOST_GENERATOR=${CMAKE_GENERATOR}"
                        "-DHOST_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
                        "-DHOST_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
                        "-DHOST_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
                        "-DPLATFORM_ID=${PINGCORE_GENERATED_LIBRARY_PLATFORM}"
                        -P "${CMAKE_CURRENT_SOURCE_DIR}/generated_library.cmake"
        )
        if (MSVC AND CMAKE_GENERATOR MATCHES "^Ninja")
            set_tests_properties(PingCoreGeneratedLibraryTests PROPERTIES
                    ENVIRONMENT "${PINGCORE_SOURCE_SUBDIRECTORY_ENVIRONMENT}"
            )
        endif ()
    endif ()
endif ()

if (TARGET pingcore_cli AND NOT PINGCORE_ENABLE_PROFILING
        AND (WIN32 OR APPLE OR (UNIX AND NOT APPLE AND NOT EMSCRIPTEN)))
    if (WIN32)
        set(PINGCORE_INSTALLED_CONSUMER_PLATFORM windows)
    elseif (APPLE)
        set(PINGCORE_INSTALLED_CONSUMER_PLATFORM macos)
    else ()
        set(PINGCORE_INSTALLED_CONSUMER_PLATFORM linux)
    endif ()
    # SDK discovery must run outside the source tree and any ancestor application project.
    if (WIN32)
        set(PINGCORE_INSTALLED_CONSUMER_WORK_DIRECTORY "$ENV{TEMP}")
    elseif (DEFINED ENV{TMPDIR} AND NOT "$ENV{TMPDIR}" STREQUAL "")
        set(PINGCORE_INSTALLED_CONSUMER_WORK_DIRECTORY "$ENV{TMPDIR}")
    else ()
        set(PINGCORE_INSTALLED_CONSUMER_WORK_DIRECTORY "/tmp")
    endif ()
    add_test(NAME PingCoreInstalledConsumerTests
            COMMAND "${CMAKE_COMMAND}"
                    "-DBUILD_DIRECTORY=${PROJECT_BINARY_DIR}"
                    "-DBUILD_CONFIG=$<CONFIG>"
                    "-DWORK_DIRECTORY=${PINGCORE_INSTALLED_CONSUMER_WORK_DIRECTORY}"
                    "-DSOURCE_DIRECTORY=${PINGCORE_PROJECT_DIR}"
                    "-DINSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}"
                    "-DCLI_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
                    "-DPLATFORM_ID=${PINGCORE_INSTALLED_CONSUMER_PLATFORM}"
                    "-DHOST_GENERATOR=${CMAKE_GENERATOR}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/installed_consumer.cmake"
    )
    # This test performs a fresh source build in addition to relocated SDK consumer checks.
    set_tests_properties(PingCoreInstalledConsumerTests PROPERTIES TIMEOUT 1200)
endif ()

if (TARGET pingcore_cli AND PINGCORE_SDK_PACKAGE_FILE_NAME AND NOT PINGCORE_ENABLE_PROFILING)
    if (WIN32)
        set(PINGCORE_SDK_ARCHIVE_WORK_DIRECTORY "$ENV{TEMP}")
    elseif (DEFINED ENV{TMPDIR} AND NOT "$ENV{TMPDIR}" STREQUAL "")
        set(PINGCORE_SDK_ARCHIVE_WORK_DIRECTORY "$ENV{TMPDIR}")
    else ()
        set(PINGCORE_SDK_ARCHIVE_WORK_DIRECTORY "/tmp")
    endif ()
    add_test(NAME PingCoreSdkArchiveTests
            COMMAND "${CMAKE_COMMAND}"
                    "-DBUILD_DIRECTORY=${PROJECT_BINARY_DIR}"
                    "-DBUILD_CONFIG=$<CONFIG>"
                    "-DWORK_DIRECTORY=${PINGCORE_SDK_ARCHIVE_WORK_DIRECTORY}"
                    "-DCPACK_COMMAND=${CMAKE_CPACK_COMMAND}"
                    "-DPACKAGE_FILE_NAME=${PINGCORE_SDK_PACKAGE_FILE_NAME}"
                    "-DPACKAGE_GENERATOR=${PINGCORE_SDK_PACKAGE_GENERATOR}"
                    "-DPACKAGE_EXTENSION=${PINGCORE_SDK_PACKAGE_EXTENSION}"
                    "-DINSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}"
                    "-DINSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}"
                    "-DCLI_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
                    "-DHOST_PLATFORM=${PINGCORE_SDK_HOST_PLATFORM}"
                    "-DHOST_ARCHITECTURE=${PINGCORE_SDK_HOST_ARCHITECTURE}"
                    "-DPLATFORM_ARTIFACTS_INCLUDED=$<BOOL:${PINGCORE_INTERNAL_SDK_ARTIFACT_ROOT}>"
                    "-DWINDOWS_DEBUG_INCLUDED=$<BOOL:${PINGCORE_INTERNAL_PACKAGE_WINDOWS_DEBUG}>"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/sdk_archive.cmake"
    )
    set_tests_properties(PingCoreSdkArchiveTests PROPERTIES RUN_SERIAL TRUE)

    add_test(NAME PingCoreSdkReleaseTests
            COMMAND "${CMAKE_COMMAND}"
                    "-DSOURCE_DIRECTORY=${PINGCORE_PROJECT_DIR}"
                    "-DWORK_DIRECTORY=${PINGCORE_SDK_ARCHIVE_WORK_DIRECTORY}"
                    "-DVALIDATOR_PATH=${PINGCORE_PROJECT_DIR}/cmake/ValidateSdkRelease.cmake"
                    "-DPROJECT_VERSION=${PROJECT_VERSION}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/sdk_release.cmake"
    )

    if (APPLE OR (UNIX AND NOT APPLE AND NOT EMSCRIPTEN))
        add_test(NAME PingCoreSdkInstallerTests
                COMMAND "${CMAKE_COMMAND}"
                        "-DBUILD_DIRECTORY=${PROJECT_BINARY_DIR}"
                        "-DBUILD_CONFIG=$<CONFIG>"
                        "-DWORK_DIRECTORY=${PINGCORE_SDK_ARCHIVE_WORK_DIRECTORY}"
                        "-DCPACK_COMMAND=${CMAKE_CPACK_COMMAND}"
                        "-DPACKAGE_FILE_NAME=${PINGCORE_SDK_PACKAGE_FILE_NAME}"
                        "-DPACKAGE_GENERATOR=${PINGCORE_SDK_PACKAGE_GENERATOR}"
                        "-DPACKAGE_EXTENSION=${PINGCORE_SDK_PACKAGE_EXTENSION}"
                        "-DINSTALLER_PATH=${PINGCORE_PROJECT_DIR}/install.sh"
                        "-DHOST_PLATFORM=${PINGCORE_SDK_HOST_PLATFORM}"
                        "-DHOST_ARCHITECTURE=${PINGCORE_SDK_HOST_ARCHITECTURE}"
                        -P "${CMAKE_CURRENT_SOURCE_DIR}/sdk_installer.cmake"
        )
        set_tests_properties(PingCoreSdkInstallerTests PROPERTIES RUN_SERIAL TRUE)
    endif ()
endif ()

get_property(integration_tests DIRECTORY PROPERTY TESTS)
foreach (integration_test IN LISTS integration_tests)
    set_tests_properties(${integration_test} PROPERTIES LABELS "build")
endforeach ()
if (TEST PingCoreAndroidAppIntegrationTests)
    set_tests_properties(PingCoreAndroidAppIntegrationTests PROPERTIES LABELS "build;platform;android;portable")
endif ()

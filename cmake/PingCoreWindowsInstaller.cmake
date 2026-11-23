include_guard(GLOBAL)

function(_pingcore_restore_wix_package package_name archive_sha256 destination)
    set(PINGCORE_WIX_REQUIRED_FILES ${ARGN})
    set(PINGCORE_WIX_PACKAGE_COMPLETE TRUE)
    foreach (PINGCORE_WIX_REQUIRED_FILE IN LISTS PINGCORE_WIX_REQUIRED_FILES)
        if (NOT EXISTS "${destination}/${PINGCORE_WIX_REQUIRED_FILE}")
            set(PINGCORE_WIX_PACKAGE_COMPLETE FALSE)
            break()
        endif ()
    endforeach ()
    if (PINGCORE_WIX_PACKAGE_COMPLETE)
        return()
    endif ()

    file(MAKE_DIRECTORY "${PINGCORE_WIX_ROOT}/downloads")
    string(TOLOWER "${package_name}" PINGCORE_WIX_PACKAGE_ID)
    set(PINGCORE_WIX_ARCHIVE
            "${PINGCORE_WIX_ROOT}/downloads/${PINGCORE_WIX_PACKAGE_ID}.${PINGCORE_WIX_VERSION}.nupkg"
    )
    file(DOWNLOAD
            "https://api.nuget.org/v3-flatcontainer/${PINGCORE_WIX_PACKAGE_ID}/${PINGCORE_WIX_VERSION}/${PINGCORE_WIX_PACKAGE_ID}.${PINGCORE_WIX_VERSION}.nupkg"
            "${PINGCORE_WIX_ARCHIVE}"
            EXPECTED_HASH "SHA256=${archive_sha256}"
            TLS_VERIFY ON
            SHOW_PROGRESS
    )
    file(REMOVE_RECURSE "${destination}")
    file(MAKE_DIRECTORY "${destination}")
    file(ARCHIVE_EXTRACT INPUT "${PINGCORE_WIX_ARCHIVE}" DESTINATION "${destination}")
    foreach (PINGCORE_WIX_REQUIRED_FILE IN LISTS PINGCORE_WIX_REQUIRED_FILES)
        if (NOT EXISTS "${destination}/${PINGCORE_WIX_REQUIRED_FILE}")
            message(FATAL_ERROR "Restored ${package_name} package is incomplete")
        endif ()
    endforeach ()
endfunction()

function(_pingcore_prepare_wix_dependencies)
    set(PINGCORE_WIX_VERSION "5.0.2")
    set(PINGCORE_WIX_TOOL_ARCHIVE_SHA256
            "f30ef0c74e2a986126539c5780be93ac24e8136eaf723b1937b26272703ae173"
    )
    set(PINGCORE_WIX_BOOTSTRAPPER_ARCHIVE_SHA256
            "6e0d3c68a68dcedde4a3a68de896f124a7b19c4a823fac49856e2ee77cb16256"
    )
    set(PINGCORE_WIX_DUTIL_ARCHIVE_SHA256
            "aa4f0668044318820e6c31ffef9f4141830c9fd8ebbe038281329423916547fe"
    )
    set(PINGCORE_WIX_TOOL_DIRECTORY "${PINGCORE_WIX_ROOT}/tool")
    set(PINGCORE_WIX_BOOTSTRAPPER_DIRECTORY "${PINGCORE_WIX_ROOT}/bootstrapper")
    set(PINGCORE_WIX_DUTIL_DIRECTORY "${PINGCORE_WIX_ROOT}/dutil")
    _pingcore_restore_wix_package(
            "wix"
            "${PINGCORE_WIX_TOOL_ARCHIVE_SHA256}"
            "${PINGCORE_WIX_TOOL_DIRECTORY}"
            "tools/net6.0/any/wix.exe"
    )
    _pingcore_restore_wix_package(
            "WixToolset.BootstrapperApplicationApi"
            "${PINGCORE_WIX_BOOTSTRAPPER_ARCHIVE_SHA256}"
            "${PINGCORE_WIX_BOOTSTRAPPER_DIRECTORY}"
            "build/native/include/BootstrapperApplication.h"
            "build/native/v14/x64/balutil.lib"
            "runtimes/win-x64/native/mbanative.dll"
    )
    _pingcore_restore_wix_package(
            "WixToolset.DUtil"
            "${PINGCORE_WIX_DUTIL_ARCHIVE_SHA256}"
            "${PINGCORE_WIX_DUTIL_DIRECTORY}"
            "build/native/include/dutil.h"
            "build/native/v14/x64/dutil.lib"
    )

    set(PINGCORE_WIX_EXECUTABLE "${PINGCORE_WIX_TOOL_DIRECTORY}/tools/net6.0/any/wix.exe")
    cmake_path(CONVERT "${PINGCORE_WIX_EXECUTABLE}" TO_CMAKE_PATH_LIST PINGCORE_WIX_EXECUTABLE NORMALIZE)
    execute_process(
            COMMAND "${PINGCORE_WIX_EXECUTABLE}" --version
            RESULT_VARIABLE PINGCORE_WIX_PROBE_RESULT
            OUTPUT_VARIABLE PINGCORE_WIX_PROBE_OUTPUT
            ERROR_VARIABLE PINGCORE_WIX_PROBE_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
    )
    if (NOT PINGCORE_WIX_PROBE_RESULT STREQUAL "0")
        if (NOT PINGCORE_WIX_PROBE_ERROR)
            set(PINGCORE_WIX_PROBE_ERROR "${PINGCORE_WIX_PROBE_OUTPUT}")
        endif ()
        message(FATAL_ERROR
                "PingCore Windows packaging requires Microsoft.NETCore.App 6.0 or newer to run WiX: ${PINGCORE_WIX_PROBE_ERROR}"
        )
    endif ()

    set(PINGCORE_WIX_EXECUTABLE "${PINGCORE_WIX_EXECUTABLE}" PARENT_SCOPE)
    set(PINGCORE_WIX_BOOTSTRAPPER_INCLUDE
            "${PINGCORE_WIX_BOOTSTRAPPER_DIRECTORY}/build/native/include"
            PARENT_SCOPE
    )
    set(PINGCORE_WIX_BOOTSTRAPPER_LIBRARY
            "${PINGCORE_WIX_BOOTSTRAPPER_DIRECTORY}/build/native/v14/x64/balutil.lib"
            PARENT_SCOPE
    )
    set(PINGCORE_WIX_BOOTSTRAPPER_RUNTIME
            "${PINGCORE_WIX_BOOTSTRAPPER_DIRECTORY}/runtimes/win-x64/native/mbanative.dll"
            PARENT_SCOPE
    )
    set(PINGCORE_WIX_DUTIL_INCLUDE
            "${PINGCORE_WIX_DUTIL_DIRECTORY}/build/native/include"
            PARENT_SCOPE
    )
    set(PINGCORE_WIX_DUTIL_LIBRARY
            "${PINGCORE_WIX_DUTIL_DIRECTORY}/build/native/v14/x64/dutil.lib"
            PARENT_SCOPE
    )
endfunction()

function(pingcore_add_windows_installer target_name)
    if (NOT WIN32)
        message(FATAL_ERROR "pingcore_add_windows_installer() is available only on Windows")
    endif ()
    if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "PingCore Windows installer packaging currently supports x64 only")
    endif ()
    if (NOT PINGCORE_WIX_ROOT)
        message(FATAL_ERROR "pingcore_add_windows_installer() requires PINGCORE_WIX_ROOT")
    endif ()

    cmake_parse_arguments(PINGCORE_INSTALLER
            ""
            "RESOURCE_NAMESPACE;INTEGRATION_OUTPUT"
            "SOURCES;RESOURCES"
            ${ARGN}
    )
    if (NOT PINGCORE_INSTALLER_SOURCES)
        message(FATAL_ERROR "pingcore_add_windows_installer() requires SOURCES")
    endif ()
    if (NOT PINGCORE_INSTALLER_INTEGRATION_OUTPUT)
        message(FATAL_ERROR "pingcore_add_windows_installer() requires INTEGRATION_OUTPUT")
    endif ()
    _pingcore_prepare_wix_dependencies()

    if (EXISTS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../platform/windows/windows_installer.cpp")
        set(PINGCORE_WINDOWS_INSTALLER_SOURCE_DIRECTORY
                "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../platform/windows"
        )
    else ()
        set(PINGCORE_WINDOWS_INSTALLER_SOURCE_DIRECTORY "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    endif ()

    pingcore_add_app(${target_name}
            SOURCES
                ${PINGCORE_INSTALLER_SOURCES}
                "${PINGCORE_WINDOWS_INSTALLER_SOURCE_DIRECTORY}/windows_installer.cpp"
            RESOURCES
                ${PINGCORE_INSTALLER_RESOURCES}
            RESOURCE_NAMESPACE
                "${PINGCORE_INSTALLER_RESOURCE_NAMESPACE}"
    )
    target_include_directories(${target_name} PRIVATE
            "${PINGCORE_WINDOWS_INSTALLER_SOURCE_DIRECTORY}"
            "${PINGCORE_WIX_BOOTSTRAPPER_INCLUDE}"
            "${PINGCORE_WIX_DUTIL_INCLUDE}"
    )
    target_link_libraries(${target_name} PRIVATE
            "${PINGCORE_WIX_BOOTSTRAPPER_LIBRARY}"
            "${PINGCORE_WIX_DUTIL_LIBRARY}"
            version
    )
    target_compile_definitions(${target_name} PRIVATE
            UNICODE
            _UNICODE
            NOMINMAX
            WIN32_LEAN_AND_MEAN
    )
    set_property(TARGET ${target_name} PROPERTY PINGCORE_WINDOWS_CRT_ENTRY wWinMainCRTStartup)
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${PINGCORE_WIX_BOOTSTRAPPER_RUNTIME}"
                    "$<TARGET_FILE_DIR:${target_name}>/mbanative.dll"
            VERBATIM
    )

    get_target_property(PINGCORE_INSTALLER_RESOURCE_PACKAGE
            ${target_name}
            PINGCORE_RESOURCE_PACKAGE
    )
    set(PINGCORE_INSTALLER_COMPONENT "PingCoreInstaller_${target_name}")
    set_property(TARGET ${target_name} PROPERTY PINGCORE_APPLICATION_INSTALL_COMPONENT "${PINGCORE_INSTALLER_COMPONENT}")
    install(TARGETS ${target_name} RUNTIME DESTINATION . COMPONENT "${PINGCORE_INSTALLER_COMPONENT}")
    install(DIRECTORY "${PINGCORE_INSTALLER_RESOURCE_PACKAGE}/"
            DESTINATION "$<TARGET_FILE_BASE_NAME:${target_name}>.resources"
            COMPONENT "${PINGCORE_INSTALLER_COMPONENT}"
    )
    pingcore_add_runtime_dependencies(${target_name} FILES "${PINGCORE_WIX_BOOTSTRAPPER_RUNTIME}")
    _pingcore_install_runtime_dependencies(${target_name} "${PINGCORE_INSTALLER_COMPONENT}"
            . "$<TARGET_FILE_NAME:${target_name}>"
    )
    file(GENERATE
            OUTPUT "${PINGCORE_INSTALLER_INTEGRATION_OUTPUT}"
            CONTENT "{\n  \"schema\": 1,\n  \"wix\": \"${PINGCORE_WIX_EXECUTABLE}\",\n  \"installer\": \"$<TARGET_FILE_NAME:${target_name}>\",\n  \"installComponent\": \"${PINGCORE_INSTALLER_COMPONENT}\"\n}\n"
    )
endfunction()

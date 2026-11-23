if (PINGCORE_PROJECT_PLAN_OUTPUT)
    get_filename_component(PINGCORE_PROJECT_PLAN_DIRECTORY
            "${PINGCORE_PROJECT_PLAN_OUTPUT}"
            DIRECTORY
    )
    file(MAKE_DIRECTORY "${PINGCORE_PROJECT_PLAN_DIRECTORY}")
    file(WRITE "${PINGCORE_PROJECT_PLAN_OUTPUT}" [=[
{
  "schema": 1,
  "kind": "app",
  "name": "CTS App Minimal",
  "id": "org.pingcore.cts.appminimal",
  "target": "cts_app_minimal"
}
]=])
endif ()

if (PINGCORE_PROJECT_PLAN_ONLY)
    return()
endif ()

if (NOT PINGCORE_LIBRARY_GRAPH_ONLY)
    enable_language(CXX)
    if (WIN32)
        enable_language(RC)
    endif ()
endif ()

if (NOT TARGET PingCore::pingcore AND NOT TARGET PingCore::pingcore_static)
    set(PINGCORE_HOME "$ENV{PINGCORE_HOME}" CACHE PATH "PingCore framework directory")
    if (PINGCORE_HOME AND EXISTS "${PINGCORE_HOME}/CMakeLists.txt"
            AND EXISTS "${PINGCORE_HOME}/include/pingcore/pingcore.h")
        set(PINGCORE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(PINGCORE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(PINGCORE_BUILD_SHARED OFF CACHE BOOL "" FORCE)
        set(PINGCORE_BUILD_STATIC ON CACHE BOOL "" FORCE)
        add_subdirectory("${PINGCORE_HOME}" "${CMAKE_BINARY_DIR}/pingcore-sdk" EXCLUDE_FROM_ALL)
    elseif (PINGCORE_HOME)
        find_package(PingCore CONFIG REQUIRED
                PATHS "${PINGCORE_HOME}"
                NO_DEFAULT_PATH
                NO_CMAKE_FIND_ROOT_PATH
        )
    else ()
        find_package(PingCore CONFIG REQUIRED)
    endif ()
endif ()

if (NOT PINGCORE_LIBRARY_GRAPH_ONLY)
    if (WIN32)
        include("${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/pingcore.cmake" OPTIONAL)
    elseif (APPLE AND NOT IOS)
        include("${CMAKE_CURRENT_SOURCE_DIR}/platform/macos/pingcore.cmake" OPTIONAL)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        include("${CMAKE_CURRENT_SOURCE_DIR}/platform/linux/pingcore.cmake" OPTIONAL)
    elseif (EMSCRIPTEN)
        include("${CMAKE_CURRENT_SOURCE_DIR}/platform/web/pingcore.cmake" OPTIONAL)
    endif ()
endif ()

set(PINGCORE_APP_RESOURCE_OUTPUT_ARGUMENTS)
if (ANDROID AND PINGCORE_ANDROID_RESOURCE_OUTPUT_ROOT)
    list(APPEND PINGCORE_APP_RESOURCE_OUTPUT_ARGUMENTS
            RESOURCE_OUTPUT_DIRECTORY
            "${PINGCORE_ANDROID_RESOURCE_OUTPUT_ROOT}/${ANDROID_ABI}"
    )
endif ()

function(pingcore_configure_project_app target_name)
    if (NOT TARGET ${target_name})
        message(FATAL_ERROR "pingcore_configure_project_app() target does not exist: ${target_name}")
    endif ()
    if (PINGCORE_LIBRARY_GRAPH_ONLY)
        return()
    endif ()

    get_target_property(PINGCORE_PROJECT_APP_INSTALL_COMPONENT
            ${target_name}
            PINGCORE_APPLICATION_INSTALL_COMPONENT
    )
    if (NOT PINGCORE_PROJECT_APP_INSTALL_COMPONENT
            OR PINGCORE_PROJECT_APP_INSTALL_COMPONENT MATCHES "-NOTFOUND$")
        message(FATAL_ERROR "PingCore application target is missing its install component: ${target_name}")
    endif ()

    if (WIN32)
        target_sources(${target_name} PRIVATE
                "${PINGCORE_WINDOWS_MANIFEST}"
                "${PINGCORE_WINDOWS_RESOURCE}"
        )
    endif ()
    if (WIN32 AND COMMAND pingcore_configure_windows_project_package)
        pingcore_configure_windows_project_package(
                ${target_name}
                "${PINGCORE_PROJECT_APP_INSTALL_COMPONENT}"
        )
    endif ()
    if (APPLE AND NOT IOS AND PINGCORE_MACOS_INFO_PLIST)
        set_target_properties(${target_name} PROPERTIES
                MACOSX_BUNDLE_INFO_PLIST "${PINGCORE_MACOS_INFO_PLIST}"
                MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
                MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        )
    endif ()
    if (APPLE AND NOT IOS AND COMMAND pingcore_configure_macos_project_package)
        pingcore_configure_macos_project_package(
                ${target_name}
                "${PINGCORE_PROJECT_APP_INSTALL_COMPONENT}"
        )
    endif ()
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND COMMAND pingcore_configure_linux_project_package)
        pingcore_configure_linux_project_package(
                ${target_name}
                "${PINGCORE_PROJECT_APP_INSTALL_COMPONENT}"
        )
    endif ()
    if (EMSCRIPTEN AND COMMAND pingcore_configure_web_app)
        pingcore_configure_web_app(${target_name})
    endif ()
endfunction()

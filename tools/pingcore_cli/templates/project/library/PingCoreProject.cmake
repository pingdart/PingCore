if (PINGCORE_PROJECT_PLAN_OUTPUT)
    get_filename_component(PINGCORE_PROJECT_PLAN_DIRECTORY
            "${PINGCORE_PROJECT_PLAN_OUTPUT}"
            DIRECTORY
    )
    file(MAKE_DIRECTORY "${PINGCORE_PROJECT_PLAN_DIRECTORY}")
    file(WRITE "${PINGCORE_PROJECT_PLAN_OUTPUT}" [=[
{
  "schema": 1,
  "kind": "library",
  "name": "@PROJECT_NAME@",
  "id": "@PROJECT_ID@",
  "target": "@TARGET_NAME@",
  "namespace": "@LIBRARY_NAMESPACE@",
  "publicTarget": "@LIBRARY_PUBLIC_TARGET@"
}
]=])
endif ()

if (PINGCORE_PROJECT_PLAN_ONLY)
    return()
endif ()

if (NOT PINGCORE_LIBRARY_GRAPH_ONLY)
    enable_language(CXX)
    if (APPLE AND NOT IOS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/platform/macos/src")
        enable_language(OBJCXX)
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

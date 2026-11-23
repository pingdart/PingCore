cmake_minimum_required(VERSION 3.20)
project(pingcore_android_example_runner LANGUAGES CXX)

if (NOT PINGCORE_ROOT_DIR)
    message(FATAL_ERROR "PINGCORE_ROOT_DIR is required")
endif ()
file(TO_CMAKE_PATH "${PINGCORE_ROOT_DIR}" PINGCORE_ROOT_DIR)

set(PINGCORE_PROJECT_DIR "${PINGCORE_ROOT_DIR}")
set(PINGCORE_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PINGCORE_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(PINGCORE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PINGCORE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${PINGCORE_ROOT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/pingcore" EXCLUDE_FROM_ALL)
include("${PINGCORE_ROOT_DIR}/cmake/PingCoreExamples.cmake")

set(PINGCORE_EXAMPLE "ui_gallery" CACHE STRING "Example displayed by the Android example runner")
if (NOT PINGCORE_EXAMPLE MATCHES "^[a-z0-9_]+$")
    message(FATAL_ERROR "PINGCORE_EXAMPLE must be a snake-case example directory name")
endif ()

set(PINGCORE_EXAMPLE_DIR
        "${PINGCORE_ROOT_DIR}/examples/${PINGCORE_EXAMPLE}"
)
if (NOT EXISTS "${PINGCORE_EXAMPLE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
            "PingCore Android example does not exist: ${PINGCORE_EXAMPLE}"
    )
endif ()

add_subdirectory(
        "${PINGCORE_EXAMPLE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/example"
)

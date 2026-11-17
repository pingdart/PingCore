# cmake/Dependencies.cmake
# PingDart Framework - Cryptographically Verified Dependency Management (ENG-102)

include(FetchContent)
find_package(Threads REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Freetype QUIET)
find_package(HarfBuzz QUIET)

if(NOT TARGET Freetype::Freetype)
    add_library(Freetype::Freetype INTERFACE IMPORTED GLOBAL)
endif()

if(NOT TARGET HarfBuzz::HarfBuzz)
    add_library(HarfBuzz::HarfBuzz INTERFACE IMPORTED GLOBAL)
endif()

option(PINGDART_OFFLINE_BUILD "Disallow network access and require all dependencies in local cache" OFF)

set(PINGDART_CACHE_DIR "${CMAKE_SOURCE_DIR}/.cache" CACHE PATH "Directory storing pre-downloaded dependency archives")

# Resolves a cryptographically locked archive dependency
function(pingdart_resolve_archive_dependency)
    set(options)
    set(oneValueArgs NAME URL SHA256 CACHE_FILE)
    set(multiValueArgs)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_NAME OR NOT ARG_URL OR NOT ARG_SHA256 OR NOT ARG_CACHE_FILE)
        message(FATAL_ERROR "pingdart_resolve_archive_dependency requires NAME, URL, SHA256, and CACHE_FILE")
    endif()

    set(LOCAL_ARCHIVE_PATH "${PINGDART_CACHE_DIR}/${ARG_CACHE_FILE}")

    if(EXISTS "${LOCAL_ARCHIVE_PATH}")
        message(STATUS "[Dependencies] Found local cached archive for ${ARG_NAME}: ${LOCAL_ARCHIVE_PATH}")
        file(SHA256 "${LOCAL_ARCHIVE_PATH}" COMPUTED_HASH)
        if(NOT COMPUTED_HASH STREQUAL ARG_SHA256)
            message(FATAL_ERROR "Security violation: Cached archive for '${ARG_NAME}' at ${LOCAL_ARCHIVE_PATH} is corrupted!\n"
                                "Expected SHA-256: ${ARG_SHA256}\n"
                                "Found SHA-256:    ${COMPUTED_HASH}")
        endif()
        set(RESOLVED_URL "${LOCAL_ARCHIVE_PATH}")
    else()
        if(PINGDART_OFFLINE_BUILD OR DEFINED ENV{PINGDART_OFFLINE_BUILD})
            message(WARNING "[Dependencies] Offline build: '${ARG_NAME}' not in cache (${LOCAL_ARCHIVE_PATH}). Skipping remote download.")
            return()
        endif()
        message(STATUS "[Dependencies] Downloading ${ARG_NAME} from ${ARG_URL}...")
        set(RESOLVED_URL "${ARG_URL}")
    endif()

    FetchContent_Declare(
        ${ARG_NAME}
        URL "${RESOLVED_URL}"
        URL_HASH "SHA256=${ARG_SHA256}"
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        TIMEOUT 15
    )

    FetchContent_MakeAvailable(${ARG_NAME})
endfunction()

# -----------------------------------------------------------------------------
# Pinned Dependency Declarations (from pingdart.lock)
# -----------------------------------------------------------------------------

# GoogleTest v1.15.2 (Required for testing)
if(BUILD_TESTING)
    # Check for system-installed GoogleTest first
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "[Dependencies] Found system GoogleTest: ${GTEST_VERSION}")
    else()
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        pingdart_resolve_archive_dependency(
            NAME googletest
            URL "https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz"
            SHA256 "7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926"
            CACHE_FILE "googletest-v1.15.2.tar.gz"
        )
    endif()
endif()

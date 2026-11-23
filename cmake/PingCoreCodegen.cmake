include_guard(GLOBAL)

function(_pingcore_resolve_host output_platform output_architecture)
    string(TOLOWER "${CMAKE_HOST_SYSTEM_NAME}" host_platform)
    if (host_platform STREQUAL "android"
            OR (host_platform STREQUAL "linux"
            AND (NOT "$ENV{TERMUX_VERSION}" STREQUAL ""
            OR (NOT "$ENV{ANDROID_ROOT}" STREQUAL ""
            AND EXISTS "/system/bin/getprop"))))
        set(host_platform "android")
    elseif (host_platform STREQUAL "darwin")
        set(host_platform "macos")
    elseif (NOT host_platform STREQUAL "windows"
            AND NOT host_platform STREQUAL "linux")
        message(FATAL_ERROR
                "PingCore host tools do not support ${CMAKE_HOST_SYSTEM_NAME}"
        )
    endif ()

    string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" host_architecture)
    if (host_architecture MATCHES "^(amd64|x64|x86_64)$")
        set(host_architecture "x86_64")
    elseif (host_architecture MATCHES "^(aarch64|arm64)$")
        if (host_platform STREQUAL "linux")
            set(host_architecture "aarch64")
        elseif (host_platform STREQUAL "android")
            set(host_architecture "arm64-v8a")
        else ()
            set(host_architecture "arm64")
        endif ()
    else ()
        message(FATAL_ERROR
                "PingCore host tools do not support ${CMAKE_HOST_SYSTEM_PROCESSOR}"
        )
    endif ()

    set(${output_platform} "${host_platform}" PARENT_SCOPE)
    set(${output_architecture} "${host_architecture}" PARENT_SCOPE)
endfunction()

function(pingcore_resolve_host_tool tool_name output_variable)
    _pingcore_resolve_host(PINGCORE_HOST_PLATFORM PINGCORE_HOST_ARCHITECTURE)

    set(PINGCORE_HOST_TOOL_SUFFIX)
    if (PINGCORE_HOST_PLATFORM STREQUAL "windows")
        set(PINGCORE_HOST_TOOL_SUFFIX ".exe")
    endif ()
    if (PINGCORE_HOST_TOOL_ROOT)
        set(PINGCORE_RESOLVED_HOST_TOOL_ROOT "${PINGCORE_HOST_TOOL_ROOT}")
    else ()
        # Source builds resolve tools beside this module; an installed package supplies PINGCORE_HOST_TOOL_ROOT.
        get_filename_component(PINGCORE_RESOLVED_HOST_TOOL_ROOT
                "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/prebuilt"
                ABSOLUTE
        )
    endif ()
    set(PINGCORE_HOST_TOOL_DIRECTORY
            "${PINGCORE_RESOLVED_HOST_TOOL_ROOT}/${PINGCORE_HOST_PLATFORM}/${PINGCORE_HOST_ARCHITECTURE}"
    )
    set(PINGCORE_HOST_TOOL
            "${PINGCORE_HOST_TOOL_DIRECTORY}/${tool_name}${PINGCORE_HOST_TOOL_SUFFIX}"
    )
    if (NOT EXISTS "${PINGCORE_HOST_TOOL}")
        message(FATAL_ERROR
                "PingCore host tool is missing: ${PINGCORE_HOST_TOOL}"
        )
    endif ()
    set(${output_variable} "${PINGCORE_HOST_TOOL}" PARENT_SCOPE)
endfunction()

function(pingcore_enable_codegen target_name)
    if (NOT TARGET ${target_name})
        message(FATAL_ERROR
                "pingcore_enable_codegen() target does not exist: ${target_name}"
        )
    endif ()

    get_target_property(PINGCORE_CODEGEN_ALREADY_SCHEDULED
            ${target_name}
            PINGCORE_CODEGEN_SCHEDULED
    )
    if (PINGCORE_CODEGEN_ALREADY_SCHEDULED)
        return()
    endif ()
    set_property(TARGET ${target_name} PROPERTY PINGCORE_CODEGEN_SCHEDULED TRUE)
    # DEFER evaluates arguments after function locals expire, so bind the target name into the deferred call now.
    cmake_language(EVAL CODE
            "cmake_language(DEFER CALL _pingcore_apply_codegen [[${target_name}]])"
    )
endfunction()

function(_pingcore_apply_codegen target_name)
    pingcore_resolve_host_tool("hcg" PINGCORE_CODEGEN_COMMAND)

    get_target_property(PINGCORE_CODEGEN_ALREADY_ENABLED
            ${target_name}
            PINGCORE_CODEGEN_ENABLED
    )
    if (PINGCORE_CODEGEN_ALREADY_ENABLED)
        return()
    endif ()

    get_target_property(PINGCORE_CODEGEN_TARGET_TYPE ${target_name} TYPE)
    if (PINGCORE_CODEGEN_TARGET_TYPE STREQUAL "INTERFACE_LIBRARY"
            OR PINGCORE_CODEGEN_TARGET_TYPE STREQUAL "UTILITY")
        message(FATAL_ERROR
                "pingcore_enable_codegen() requires a compilable target: ${target_name}"
        )
    endif ()

    get_target_property(PINGCORE_CODEGEN_SOURCE_DIR ${target_name} SOURCE_DIR)
    get_target_property(PINGCORE_CODEGEN_BINARY_DIR ${target_name} BINARY_DIR)
    get_target_property(PINGCORE_CODEGEN_SOURCES ${target_name} SOURCES)

    if (NOT PINGCORE_CODEGEN_SOURCES)
        message(FATAL_ERROR
                "pingcore_enable_codegen() target has no sources: ${target_name}"
        )
    endif ()

    set(PINGCORE_CODEGEN_REWRITTEN_SOURCES)
    set(PINGCORE_CODEGEN_GENERATED_SOURCES)
    set(PINGCORE_CODEGEN_PROFILING_ARGUMENTS)
    if (TARGET PingCore::pingcore)
        get_target_property(PINGCORE_CODEGEN_PROFILING PingCore::pingcore PINGCORE_SOURCE_PROFILING)
        if (PINGCORE_CODEGEN_PROFILING)
            set(PINGCORE_CODEGEN_PROFILING_ARGUMENTS --profile-source-root "${CMAKE_SOURCE_DIR}")
        endif ()
    endif ()

    foreach (PINGCORE_CODEGEN_SOURCE IN LISTS PINGCORE_CODEGEN_SOURCES)
        if (PINGCORE_CODEGEN_SOURCE MATCHES "^\\$<")
            list(APPEND PINGCORE_CODEGEN_REWRITTEN_SOURCES
                    "${PINGCORE_CODEGEN_SOURCE}"
            )
            continue()
        endif ()

        get_filename_component(PINGCORE_CODEGEN_EXTENSION
                "${PINGCORE_CODEGEN_SOURCE}"
                EXT
        )
        string(TOLOWER
                "${PINGCORE_CODEGEN_EXTENSION}"
                PINGCORE_CODEGEN_EXTENSION
        )
        if (NOT PINGCORE_CODEGEN_EXTENSION STREQUAL ".cpp"
                AND NOT PINGCORE_CODEGEN_EXTENSION STREQUAL ".cc"
                AND NOT PINGCORE_CODEGEN_EXTENSION STREQUAL ".cxx")
            list(APPEND PINGCORE_CODEGEN_REWRITTEN_SOURCES
                    "${PINGCORE_CODEGEN_SOURCE}"
            )
            continue()
        endif ()

        get_filename_component(PINGCORE_CODEGEN_ABSOLUTE_SOURCE
                "${PINGCORE_CODEGEN_SOURCE}"
                ABSOLUTE
                BASE_DIR "${PINGCORE_CODEGEN_SOURCE_DIR}"
        )
        if (NOT EXISTS "${PINGCORE_CODEGEN_ABSOLUTE_SOURCE}")
            list(APPEND PINGCORE_CODEGEN_REWRITTEN_SOURCES
                    "${PINGCORE_CODEGEN_SOURCE}"
            )
            continue()
        endif ()

        set_property(DIRECTORY "${PINGCORE_CODEGEN_SOURCE_DIR}"
                APPEND
                PROPERTY CMAKE_CONFIGURE_DEPENDS
                "${PINGCORE_CODEGEN_ABSOLUTE_SOURCE}"
        )
        file(READ
                "${PINGCORE_CODEGEN_ABSOLUTE_SOURCE}"
                PINGCORE_CODEGEN_SOURCE_CONTENT
        )
        string(FIND
                "${PINGCORE_CODEGEN_SOURCE_CONTENT}"
                "[[pingcore::composable]]"
                PINGCORE_CODEGEN_MARKER_INDEX
        )
        string(FIND
                "${PINGCORE_CODEGEN_SOURCE_CONTENT}"
                "Use"
                PINGCORE_CODEGEN_COMPOSITION_CALL_INDEX
        )
        if (PINGCORE_CODEGEN_MARKER_INDEX EQUAL -1
                AND PINGCORE_CODEGEN_COMPOSITION_CALL_INDEX EQUAL -1)
            list(APPEND PINGCORE_CODEGEN_REWRITTEN_SOURCES
                    "${PINGCORE_CODEGEN_SOURCE}"
            )
            continue()
        endif ()

        string(SHA256
                PINGCORE_CODEGEN_SOURCE_HASH
                "${PINGCORE_CODEGEN_ABSOLUTE_SOURCE}"
        )
        string(SUBSTRING
                "${PINGCORE_CODEGEN_SOURCE_HASH}"
                0
                16
                PINGCORE_CODEGEN_SOURCE_HASH
        )
        get_filename_component(PINGCORE_CODEGEN_SOURCE_NAME
                "${PINGCORE_CODEGEN_ABSOLUTE_SOURCE}"
                NAME
        )
        get_filename_component(PINGCORE_CODEGEN_SOURCE_DIRECTORY
                "${PINGCORE_CODEGEN_ABSOLUTE_SOURCE}"
                DIRECTORY
        )

        set(PINGCORE_CODEGEN_OUTPUT_DIRECTORY
                "${PINGCORE_CODEGEN_BINARY_DIR}/hcg/${target_name}/${PINGCORE_CODEGEN_SOURCE_HASH}"
        )
        set(PINGCORE_CODEGEN_OUTPUT
                "${PINGCORE_CODEGEN_OUTPUT_DIRECTORY}/${PINGCORE_CODEGEN_SOURCE_NAME}"
        )
        add_custom_command(
                OUTPUT "${PINGCORE_CODEGEN_OUTPUT}"
                COMMAND "${PINGCORE_CODEGEN_COMMAND}"
                        --input "${PINGCORE_CODEGEN_ABSOLUTE_SOURCE}"
                        --output "${PINGCORE_CODEGEN_OUTPUT}"
                        ${PINGCORE_CODEGEN_PROFILING_ARGUMENTS}
                DEPENDS
                        "${PINGCORE_CODEGEN_ABSOLUTE_SOURCE}"
                        "${PINGCORE_CODEGEN_COMMAND}"
                COMMENT
                        "Generating PingCore composable source ${PINGCORE_CODEGEN_SOURCE_NAME}"
                VERBATIM
        )

        set_source_files_properties(
                "${PINGCORE_CODEGEN_OUTPUT}"
                PROPERTIES GENERATED TRUE
        )
        target_include_directories(${target_name}
                PRIVATE "${PINGCORE_CODEGEN_SOURCE_DIRECTORY}"
        )
        list(APPEND PINGCORE_CODEGEN_REWRITTEN_SOURCES
                "${PINGCORE_CODEGEN_OUTPUT}"
        )
        list(APPEND PINGCORE_CODEGEN_GENERATED_SOURCES
                "${PINGCORE_CODEGEN_OUTPUT}"
        )
    endforeach ()

    set_property(TARGET ${target_name}
            PROPERTY SOURCES ${PINGCORE_CODEGEN_REWRITTEN_SOURCES}
    )
    set_property(TARGET ${target_name}
            PROPERTY PINGCORE_CODEGEN_ENABLED TRUE
    )
    set_property(TARGET ${target_name}
            PROPERTY PINGCORE_CODEGEN_GENERATED_SOURCES
                     "${PINGCORE_CODEGEN_GENERATED_SOURCES}"
    )

    target_compile_options(${target_name} PRIVATE
            "$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wno-unknown-attributes>"
            "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-attributes>"
            "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/wd5030>"
    )
endfunction()

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/PingCoreResources.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/PingCoreOptimization.cmake")

function(_pingcore_configure_library_graph_framework)
    if (TARGET pingcore_library_graph_framework)
        return()
    endif ()
    add_library(pingcore_library_graph_framework INTERFACE)
    add_library(PingCore::pingcore ALIAS pingcore_library_graph_framework)
    add_library(PingCore::pingcore_static ALIAS pingcore_library_graph_framework)
endfunction()

function(_pingcore_select_framework_target output_variable)
    if (TARGET PingCore::pingcore_static)
        set(PINGCORE_FRAMEWORK_TARGET PingCore::pingcore_static)
    elseif (ANDROID)
        message(FATAL_ERROR "PingCore Android integration requires the static component")
    elseif (TARGET PingCore::pingcore)
        set(PINGCORE_FRAMEWORK_TARGET PingCore::pingcore)
    else ()
        message(FATAL_ERROR "PingCore framework target is unavailable")
    endif ()
    set(${output_variable} "${PINGCORE_FRAMEWORK_TARGET}" PARENT_SCOPE)
endfunction()

function(_pingcore_resolve_library_target requested_target output_variable)
    if (NOT TARGET ${requested_target})
        message(FATAL_ERROR
                "PingCore library target does not exist: ${requested_target}"
        )
    endif ()

    get_target_property(PINGCORE_LIBRARY_ALIASED_TARGET
            ${requested_target}
            ALIASED_TARGET
    )
    if (PINGCORE_LIBRARY_ALIASED_TARGET)
        set(PINGCORE_LIBRARY_TARGET "${PINGCORE_LIBRARY_ALIASED_TARGET}")
    else ()
        set(PINGCORE_LIBRARY_TARGET "${requested_target}")
    endif ()
    set(${output_variable} "${PINGCORE_LIBRARY_TARGET}" PARENT_SCOPE)
endfunction()

function(_pingcore_json_escape input output)
    string(REPLACE "\\" "\\\\" value "${input}")
    string(REPLACE "\"" "\\\"" value "${value}")
    string(REPLACE "\n" "\\n" value "${value}")
    string(REPLACE "\r" "\\r" value "${value}")
    string(REPLACE "\t" "\\t" value "${value}")
    set(${output} "${value}" PARENT_SCOPE)
endfunction()

function(_pingcore_collect_library_dependencies target_name dependency_path libraries_variable requested_targets_variable)
    get_property(libraries TARGET ${target_name} PROPERTY PINGCORE_LIBRARIES)
    get_property(requested_targets TARGET ${target_name} PROPERTY PINGCORE_REQUESTED_LIBRARY_TARGETS)
    list(LENGTH libraries library_count)
    list(LENGTH requested_targets requested_target_count)
    if (NOT library_count EQUAL requested_target_count)
        message(FATAL_ERROR
                "PingCore library graph target identity is inconsistent for ${target_name}"
        )
    endif ()

    list(APPEND dependency_path "${target_name}")
    foreach (library requested_target IN ZIP_LISTS libraries requested_targets)
        if (library IN_LIST dependency_path)
            list(JOIN dependency_path " -> " cycle)
            message(FATAL_ERROR "PingCore library dependency cycle: ${cycle} -> ${library}")
        endif ()
        if (library IN_LIST ${libraries_variable})
            continue()
        endif ()
        list(APPEND ${libraries_variable} "${library}")
        list(APPEND ${requested_targets_variable} "${requested_target}")
        _pingcore_collect_library_dependencies("${library}" "${dependency_path}"
                "${libraries_variable}" "${requested_targets_variable}"
        )
    endforeach ()
    set(${libraries_variable} "${${libraries_variable}}" PARENT_SCOPE)
    set(${requested_targets_variable} "${${requested_targets_variable}}" PARENT_SCOPE)
endfunction()

function(_pingcore_write_library_graph target_name)
    get_property(output_file TARGET ${target_name} PROPERTY PINGCORE_LIBRARY_GRAPH_OUTPUT)
    set(PINGCORE_GRAPH_LIBRARIES "")
    set(PINGCORE_GRAPH_REQUESTED_TARGETS "")
    _pingcore_collect_library_dependencies("${target_name}" ""
            PINGCORE_GRAPH_LIBRARIES PINGCORE_GRAPH_REQUESTED_TARGETS
    )
    set(PINGCORE_GRAPH_ENTRIES)
    foreach (PINGCORE_GRAPH_LIBRARY PINGCORE_GRAPH_REQUESTED_TARGET
            IN ZIP_LISTS PINGCORE_GRAPH_LIBRARIES PINGCORE_GRAPH_REQUESTED_TARGETS)
        get_property(PINGCORE_GRAPH_SOURCE_ROOT_SET
                TARGET ${PINGCORE_GRAPH_LIBRARY}
                PROPERTY PINGCORE_LIBRARY_SOURCE_ROOT
                SET
        )
        if (NOT PINGCORE_GRAPH_SOURCE_ROOT_SET)
            continue()
        endif ()
        get_property(PINGCORE_GRAPH_SOURCE_ROOT
                TARGET ${PINGCORE_GRAPH_LIBRARY}
                PROPERTY PINGCORE_LIBRARY_SOURCE_ROOT
        )
        _pingcore_json_escape(
                "${PINGCORE_GRAPH_REQUESTED_TARGET}"
                PINGCORE_GRAPH_JSON_TARGET
        )
        _pingcore_json_escape(
                "${PINGCORE_GRAPH_SOURCE_ROOT}"
                PINGCORE_GRAPH_JSON_SOURCE_ROOT
        )
        if (PINGCORE_GRAPH_ENTRIES)
            string(APPEND PINGCORE_GRAPH_ENTRIES ",\n")
        endif ()
        string(APPEND PINGCORE_GRAPH_ENTRIES
                "    {\"target\": \"${PINGCORE_GRAPH_JSON_TARGET}\", \"sourceRoot\": \"${PINGCORE_GRAPH_JSON_SOURCE_ROOT}\"}"
        )
    endforeach ()

    get_filename_component(PINGCORE_GRAPH_DIRECTORY
            "${output_file}"
            DIRECTORY
    )
    file(MAKE_DIRECTORY "${PINGCORE_GRAPH_DIRECTORY}")
    set(PINGCORE_GRAPH_CONTENT "{\n  \"schema\": 1,\n  \"libraries\": [\n${PINGCORE_GRAPH_ENTRIES}\n  ]\n}\n")
    file(CONFIGURE OUTPUT "${output_file}"
            CONTENT "@PINGCORE_GRAPH_CONTENT@"
            @ONLY
    )
endfunction()

function(pingcore_add_library target_name)
    cmake_parse_arguments(PINGCORE_LIBRARY
            ""
            "RESOURCE_NAMESPACE"
            "SOURCES;RESOURCES"
            ${ARGN}
    )
    if (PINGCORE_LIBRARY_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
                "pingcore_add_library() received unknown arguments: ${PINGCORE_LIBRARY_UNPARSED_ARGUMENTS}"
        )
    endif ()
    if (TARGET ${target_name})
        message(FATAL_ERROR
                "pingcore_add_library() target already exists: ${target_name}"
        )
    endif ()
    if (NOT PINGCORE_LIBRARY_SOURCES)
        message(FATAL_ERROR
                "pingcore_add_library() requires at least one source"
        )
    endif ()
    if (PINGCORE_LIBRARY_RESOURCES AND NOT PINGCORE_LIBRARY_RESOURCE_NAMESPACE)
        message(FATAL_ERROR
                "pingcore_add_library() requires RESOURCE_NAMESPACE when RESOURCES is present"
        )
    endif ()
    if (PINGCORE_LIBRARY_RESOURCE_NAMESPACE AND NOT PINGCORE_LIBRARY_RESOURCES)
        message(FATAL_ERROR
                "pingcore_add_library() RESOURCE_NAMESPACE requires RESOURCES"
        )
    endif ()
    list(LENGTH PINGCORE_LIBRARY_RESOURCES PINGCORE_LIBRARY_RESOURCE_ROOT_COUNT)
    if (PINGCORE_LIBRARY_RESOURCE_ROOT_COUNT GREATER 1)
        message(FATAL_ERROR
                "pingcore_add_library() currently accepts one resource root"
        )
    endif ()
    if (PINGCORE_LIBRARY_GRAPH_ONLY)
        add_library(${target_name} INTERFACE)
        set_property(TARGET ${target_name} PROPERTY PINGCORE_LIBRARY_GRAPH_ONLY TRUE)
    else ()
        add_library(${target_name} STATIC ${PINGCORE_LIBRARY_SOURCES})
        target_compile_features(${target_name} PRIVATE cxx_std_20)
        set_target_properties(${target_name} PROPERTIES
                CXX_EXTENSIONS OFF
                POSITION_INDEPENDENT_CODE ON
        )
    endif ()
    file(REAL_PATH
            "${CMAKE_CURRENT_SOURCE_DIR}"
            PINGCORE_LIBRARY_SOURCE_ROOT
    )
    set_property(TARGET ${target_name} PROPERTY PINGCORE_LIBRARY TRUE)
    set_property(TARGET ${target_name} PROPERTY
            PINGCORE_LIBRARY_SOURCE_ROOT
            "${PINGCORE_LIBRARY_SOURCE_ROOT}"
    )
    set_property(TARGET ${target_name} APPEND PROPERTY EXPORT_PROPERTIES
            PINGCORE_LIBRARY
    )
    if (PINGCORE_LIBRARY_GRAPH_ONLY)
        return()
    endif ()
    _pingcore_select_framework_target(PINGCORE_LIBRARY_FRAMEWORK_TARGET)
    target_link_libraries(${target_name} PRIVATE ${PINGCORE_LIBRARY_FRAMEWORK_TARGET})
    _pingcore_configure_optimization(${target_name})
    pingcore_enable_codegen(${target_name})

    if (PINGCORE_LIBRARY_RESOURCES)
        list(GET PINGCORE_LIBRARY_RESOURCES 0 PINGCORE_LIBRARY_RESOURCE_ROOT)
        pingcore_add_resources(${target_name}
                ROOT "${PINGCORE_LIBRARY_RESOURCE_ROOT}"
                NAMESPACE "${PINGCORE_LIBRARY_RESOURCE_NAMESPACE}"
        )
    endif ()
endfunction()

function(pingcore_use_library target_name)
    if (NOT TARGET ${target_name})
        message(FATAL_ERROR
                "pingcore_use_library() consumer target does not exist: ${target_name}"
        )
    endif ()
    cmake_parse_arguments(PINGCORE_USE_LIBRARY
            "PUBLIC"
            "TARGET;PATH;URL;COMMIT;TAG"
            ""
            ${ARGN}
    )
    if (PINGCORE_USE_LIBRARY_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
                "pingcore_use_library() received unknown arguments: ${PINGCORE_USE_LIBRARY_UNPARSED_ARGUMENTS}"
        )
    endif ()
    if (NOT PINGCORE_USE_LIBRARY_TARGET)
        message(FATAL_ERROR "pingcore_use_library() requires TARGET")
    endif ()
    if (PINGCORE_USE_LIBRARY_PATH AND PINGCORE_USE_LIBRARY_URL)
        message(FATAL_ERROR
                "pingcore_use_library() PATH and URL are mutually exclusive"
        )
    endif ()
    if (DEFINED PINGCORE_USE_LIBRARY_COMMIT AND DEFINED PINGCORE_USE_LIBRARY_TAG)
        message(FATAL_ERROR
                "pingcore_use_library() COMMIT and TAG are mutually exclusive"
        )
    endif ()
    if (DEFINED PINGCORE_USE_LIBRARY_COMMIT AND NOT PINGCORE_USE_LIBRARY_URL)
        message(FATAL_ERROR
                "pingcore_use_library() COMMIT requires URL"
        )
    endif ()
    if (DEFINED PINGCORE_USE_LIBRARY_TAG AND NOT PINGCORE_USE_LIBRARY_URL)
        message(FATAL_ERROR
                "pingcore_use_library() TAG requires URL"
        )
    endif ()

    set(PINGCORE_LIBRARY_TARGET_PREEXISTED FALSE)
    if (TARGET ${PINGCORE_USE_LIBRARY_TARGET})
        set(PINGCORE_LIBRARY_TARGET_PREEXISTED TRUE)
    endif ()
    set(PINGCORE_LIBRARY_ORIGIN)
    if (PINGCORE_USE_LIBRARY_PATH)
        file(REAL_PATH
                "${PINGCORE_USE_LIBRARY_PATH}"
                PINGCORE_LIBRARY_SOURCE_PATH
                BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
        if (NOT EXISTS "${PINGCORE_LIBRARY_SOURCE_PATH}/CMakeLists.txt")
            message(FATAL_ERROR
                    "pingcore_use_library() PATH does not contain CMakeLists.txt: ${PINGCORE_LIBRARY_SOURCE_PATH}"
            )
        endif ()
        set(PINGCORE_LIBRARY_ORIGIN "PATH:${PINGCORE_LIBRARY_SOURCE_PATH}")
        if (NOT TARGET ${PINGCORE_USE_LIBRARY_TARGET})
            string(SHA256 PINGCORE_LIBRARY_SOURCE_HASH
                    "${PINGCORE_LIBRARY_SOURCE_PATH}"
            )
            string(SUBSTRING "${PINGCORE_LIBRARY_SOURCE_HASH}" 0 16
                    PINGCORE_LIBRARY_SOURCE_HASH
            )
            add_subdirectory(
                    "${PINGCORE_LIBRARY_SOURCE_PATH}"
                    "${CMAKE_BINARY_DIR}/pingcore-libs/${PINGCORE_LIBRARY_SOURCE_HASH}"
            )
        endif ()
    elseif (PINGCORE_USE_LIBRARY_URL)
        if (NOT PINGCORE_USE_LIBRARY_URL MATCHES "^https://")
            message(FATAL_ERROR
                    "pingcore_use_library() URL must use HTTPS"
            )
        endif ()
        if (PINGCORE_USE_LIBRARY_URL MATCHES "^https://[^/]*@")
            message(FATAL_ERROR
                    "pingcore_use_library() URL must not contain credentials"
            )
        endif ()
        if (NOT DEFINED PINGCORE_USE_LIBRARY_COMMIT
                AND NOT DEFINED PINGCORE_USE_LIBRARY_TAG)
            message(FATAL_ERROR
                    "pingcore_use_library() URL requires COMMIT or TAG"
            )
        endif ()
        if (DEFINED PINGCORE_USE_LIBRARY_COMMIT)
            string(LENGTH "${PINGCORE_USE_LIBRARY_COMMIT}"
                    PINGCORE_LIBRARY_COMMIT_LENGTH
            )
            if (NOT PINGCORE_LIBRARY_COMMIT_LENGTH EQUAL 40
                    OR NOT PINGCORE_USE_LIBRARY_COMMIT MATCHES "^[0-9A-Fa-f]+$")
                message(FATAL_ERROR
                        "pingcore_use_library() COMMIT must be a full commit SHA"
                )
            endif ()
            set(PINGCORE_LIBRARY_CHECKOUT "${PINGCORE_USE_LIBRARY_COMMIT}")
            set(PINGCORE_LIBRARY_ORIGIN
                    "URL:${PINGCORE_USE_LIBRARY_URL}@COMMIT:${PINGCORE_USE_LIBRARY_COMMIT}"
            )
        else ()
            if (PINGCORE_USE_LIBRARY_TAG MATCHES "^refs/tags/")
                message(FATAL_ERROR
                        "pingcore_use_library() TAG must not include the refs/tags/ prefix"
                )
            endif ()
            set(PINGCORE_LIBRARY_CHECKOUT "refs/tags/${PINGCORE_USE_LIBRARY_TAG}")
            set(PINGCORE_LIBRARY_ORIGIN
                    "URL:${PINGCORE_USE_LIBRARY_URL}@TAG:${PINGCORE_USE_LIBRARY_TAG}"
            )
        endif ()
        if (NOT TARGET ${PINGCORE_USE_LIBRARY_TARGET})
            include(FetchContent)
            string(SHA256 PINGCORE_LIBRARY_SOURCE_HASH
                    "${PINGCORE_LIBRARY_ORIGIN}"
            )
            string(SUBSTRING "${PINGCORE_LIBRARY_SOURCE_HASH}" 0 16
                    PINGCORE_LIBRARY_SOURCE_HASH
            )
            set(PINGCORE_LIBRARY_FETCH_NAME
                    "pingcore_library_${PINGCORE_LIBRARY_SOURCE_HASH}"
            )
            FetchContent_Declare(${PINGCORE_LIBRARY_FETCH_NAME}
                    GIT_REPOSITORY "${PINGCORE_USE_LIBRARY_URL}"
                    GIT_TAG "${PINGCORE_LIBRARY_CHECKOUT}"
                    GIT_SHALLOW FALSE
            )
            FetchContent_MakeAvailable(${PINGCORE_LIBRARY_FETCH_NAME})
        endif ()
    endif ()

    if (NOT TARGET ${PINGCORE_USE_LIBRARY_TARGET})
        message(FATAL_ERROR
                "pingcore_use_library() acquisition did not create target: ${PINGCORE_USE_LIBRARY_TARGET}"
        )
    endif ()
    _pingcore_resolve_library_target(
            ${PINGCORE_USE_LIBRARY_TARGET}
            PINGCORE_RESOLVED_LIBRARY_TARGET
    )
    get_property(PINGCORE_TARGET_IS_LIBRARY
            TARGET ${PINGCORE_RESOLVED_LIBRARY_TARGET}
            PROPERTY PINGCORE_LIBRARY
    )
    if (NOT PINGCORE_TARGET_IS_LIBRARY)
        message(FATAL_ERROR
                "pingcore_use_library() TARGET is not a PingCore library: ${PINGCORE_USE_LIBRARY_TARGET}"
        )
    endif ()

    get_property(PINGCORE_EXISTING_LIBRARIES
            TARGET ${target_name}
            PROPERTY PINGCORE_LIBRARIES
    )
    if (PINGCORE_RESOLVED_LIBRARY_TARGET IN_LIST PINGCORE_EXISTING_LIBRARIES)
        message(FATAL_ERROR
                "pingcore_use_library() library is already used by ${target_name}: ${PINGCORE_USE_LIBRARY_TARGET}"
        )
    endif ()
    get_property(PINGCORE_EXISTING_LIBRARY_ORIGIN_SET
            TARGET ${PINGCORE_RESOLVED_LIBRARY_TARGET}
            PROPERTY PINGCORE_LIBRARY_ORIGIN
            SET
    )
    if (PINGCORE_LIBRARY_ORIGIN AND PINGCORE_EXISTING_LIBRARY_ORIGIN_SET)
        get_property(PINGCORE_EXISTING_LIBRARY_ORIGIN
                TARGET ${PINGCORE_RESOLVED_LIBRARY_TARGET}
                PROPERTY PINGCORE_LIBRARY_ORIGIN
        )
        if (NOT "${PINGCORE_EXISTING_LIBRARY_ORIGIN}" STREQUAL "${PINGCORE_LIBRARY_ORIGIN}")
            message(FATAL_ERROR
                    "pingcore_use_library() target was acquired from a different source: ${PINGCORE_USE_LIBRARY_TARGET}"
            )
        endif ()
    elseif (PINGCORE_LIBRARY_ORIGIN AND PINGCORE_LIBRARY_TARGET_PREEXISTED)
        message(FATAL_ERROR
                "pingcore_use_library() TARGET already exists without a matching acquisition origin; omit PATH and URL: ${PINGCORE_USE_LIBRARY_TARGET}"
        )
    elseif (PINGCORE_LIBRARY_ORIGIN)
        set_property(TARGET ${PINGCORE_RESOLVED_LIBRARY_TARGET} PROPERTY
                PINGCORE_LIBRARY_ORIGIN
                "${PINGCORE_LIBRARY_ORIGIN}"
        )
    endif ()

    get_property(PINGCORE_LIBRARY_GRAPH_ONLY
            TARGET ${target_name}
            PROPERTY PINGCORE_LIBRARY_GRAPH_ONLY
    )
    set_property(TARGET ${target_name} APPEND PROPERTY
            PINGCORE_LIBRARIES
            "${PINGCORE_RESOLVED_LIBRARY_TARGET}"
    )
    set_property(TARGET ${target_name} APPEND PROPERTY
            PINGCORE_REQUESTED_LIBRARY_TARGETS
            "${PINGCORE_USE_LIBRARY_TARGET}"
    )
    if (NOT PINGCORE_LIBRARY_GRAPH_ONLY)
        if (PINGCORE_USE_LIBRARY_PUBLIC)
            set(link_scope PUBLIC)
        else ()
            set(link_scope PRIVATE)
        endif ()
        target_link_libraries(${target_name} ${link_scope} ${PINGCORE_USE_LIBRARY_TARGET})
        get_target_property(consumer_directory ${target_name} BINARY_DIR)
        if (NOT consumer_directory STREQUAL CMAKE_CURRENT_BINARY_DIR)
            # A resource-free link may be declared after the consumer's directory has finished.
            _pingcore_get_library_resource_package(${PINGCORE_RESOLVED_LIBRARY_TARGET} dependency_package)
            if (NOT dependency_package)
                return()
            endif ()
        endif ()
        _pingcore_schedule_resources(${target_name})
    endif ()
endfunction()

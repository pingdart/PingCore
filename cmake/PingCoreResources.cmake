include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/PingCoreCodegen.cmake")

function(_pingcore_get_resource_output target_name output_variable)
    get_property(PINGCORE_RESOURCE_OUTPUT_DIRECTORY_SET
            TARGET ${target_name}
            PROPERTY PINGCORE_RESOURCE_OUTPUT_DIRECTORY
            SET
    )
    if (PINGCORE_RESOURCE_OUTPUT_DIRECTORY_SET)
        get_property(PINGCORE_RESOURCE_OUTPUT_DIRECTORY
                TARGET ${target_name}
                PROPERTY PINGCORE_RESOURCE_OUTPUT_DIRECTORY
        )
    else ()
        get_target_property(PINGCORE_RESOURCE_TARGET_BINARY_DIR
                ${target_name}
                BINARY_DIR
        )
        set(PINGCORE_RESOURCE_OUTPUT_DIRECTORY
                "${PINGCORE_RESOURCE_TARGET_BINARY_DIR}/pingcore-resources/${target_name}"
        )
    endif ()
    set(${output_variable} "${PINGCORE_RESOURCE_OUTPUT_DIRECTORY}" PARENT_SCOPE)
endfunction()

function(_pingcore_compile_builtin_resources target_name output_variable)
    set(PINGCORE_BUILTIN_RESOURCE_OUTPUT
            "${PROJECT_BINARY_DIR}/pingcore-builtin-resources"
    )
    set(PINGCORE_BUILTIN_RESOURCE_PACKAGE
            "${PINGCORE_BUILTIN_RESOURCE_OUTPUT}/package"
    )
    set(PINGCORE_BUILTIN_RESOURCE_HEADER
            "${PINGCORE_BUILTIN_RESOURCE_OUTPUT}/include/pingcore_builtin_resources.h"
    )
    set(PINGCORE_BUILTIN_RESOURCE_INDEX
            "${PINGCORE_BUILTIN_RESOURCE_PACKAGE}/pingcore/resources.bin"
    )
    file(GLOB_RECURSE PINGCORE_BUILTIN_RESOURCE_INPUTS
            CONFIGURE_DEPENDS
            LIST_DIRECTORIES FALSE
            "${PINGCORE_PROJECT_DIR}/resources/*"
    )
    set(PINGCORE_BUILTIN_RESOURCE_PLAN
            "${PINGCORE_BUILTIN_RESOURCE_OUTPUT}/resource-plan-$<CONFIG>.txt"
    )
    file(GENERATE
            OUTPUT "${PINGCORE_BUILTIN_RESOURCE_PLAN}"
            CONTENT "inputs=${PINGCORE_BUILTIN_RESOURCE_INPUTS}\n"
    )
    pingcore_resolve_host_tool("hrc" PINGCORE_BUILTIN_RESOURCE_COMPILER_COMMAND)
    add_custom_command(
            OUTPUT
                    "${PINGCORE_BUILTIN_RESOURCE_HEADER}"
                    "${PINGCORE_BUILTIN_RESOURCE_INDEX}"
            COMMAND "${PINGCORE_BUILTIN_RESOURCE_COMPILER_COMMAND}"
                    --root "${PINGCORE_PROJECT_DIR}/resources"
                    --output "${PINGCORE_BUILTIN_RESOURCE_OUTPUT}"
                    --namespace pingcore
                    --header-name pingcore_builtin_resources.h
            DEPENDS
                    ${PINGCORE_BUILTIN_RESOURCE_INPUTS}
                    "${PINGCORE_BUILTIN_RESOURCE_PLAN}"
                    "${PINGCORE_BUILTIN_RESOURCE_COMPILER_COMMAND}"
            COMMENT "Generating PingCore built-in resources"
            VERBATIM
    )
    set_source_files_properties(
            "${PINGCORE_BUILTIN_RESOURCE_HEADER}"
            PROPERTIES
                    GENERATED TRUE
                    HEADER_FILE_ONLY TRUE
    )
    target_sources(${target_name} PRIVATE
            "${PINGCORE_BUILTIN_RESOURCE_HEADER}"
    )
    target_include_directories(${target_name} PRIVATE
            "${PINGCORE_BUILTIN_RESOURCE_OUTPUT}/include"
    )
    set(${output_variable} "${PINGCORE_BUILTIN_RESOURCE_PACKAGE}" PARENT_SCOPE)
endfunction()

function(_pingcore_get_library_resource_package target_name output_variable)
    get_property(package TARGET ${target_name} PROPERTY PINGCORE_RESOURCE_PACKAGE)
    if (NOT package)
        # A forwarding library may be configured after its consumer in the same directory.
        set(dependencies "")
        set(requested_targets "")
        _pingcore_collect_library_dependencies("${target_name}" "" dependencies requested_targets)
        foreach (dependency IN LISTS dependencies)
            get_property(dependency_package TARGET ${dependency} PROPERTY PINGCORE_RESOURCE_PACKAGE)
            if (dependency_package)
                _pingcore_get_resource_output(${target_name} resource_output)
                set(package "${resource_output}/package")
                break()
            endif ()
        endforeach ()
    endif ()
    set(${output_variable} "${package}" PARENT_SCOPE)
endfunction()

function(_pingcore_configure_resources target_name)
    get_property(PINGCORE_RESOURCE_PACKAGE_INPUTS
            TARGET ${target_name}
            PROPERTY PINGCORE_RESOURCE_PACKAGE_INPUTS
    )
    get_property(PINGCORE_RESOURCE_ROOTS
            TARGET ${target_name}
            PROPERTY PINGCORE_RESOURCE_ROOTS
    )
    get_property(PINGCORE_RESOURCE_NAMESPACES
            TARGET ${target_name}
            PROPERTY PINGCORE_RESOURCE_NAMESPACES
    )
    get_property(libraries TARGET ${target_name} PROPERTY PINGCORE_LIBRARIES)
    foreach (library IN LISTS libraries)
        _pingcore_get_library_resource_package(${library} package)
        if (package)
            list(APPEND PINGCORE_RESOURCE_PACKAGE_INPUTS "${package}")
        endif ()
    endforeach ()
    if (NOT PINGCORE_RESOURCE_PACKAGE_INPUTS AND NOT PINGCORE_RESOURCE_ROOTS)
        return()
    endif ()

    set(PINGCORE_RESOURCE_PACKAGE_INDEXES)
    foreach (PINGCORE_RESOURCE_PACKAGE IN LISTS PINGCORE_RESOURCE_PACKAGE_INPUTS)
        list(APPEND PINGCORE_RESOURCE_PACKAGE_INDEXES
                "${PINGCORE_RESOURCE_PACKAGE}/pingcore/resources.bin"
        )
    endforeach ()

    set(PINGCORE_RESOURCE_INPUTS)
    foreach (PINGCORE_RESOURCE_ROOT IN LISTS PINGCORE_RESOURCE_ROOTS)
        file(GLOB_RECURSE PINGCORE_RESOURCE_ROOT_INPUTS
                CONFIGURE_DEPENDS
                LIST_DIRECTORIES FALSE
                "${PINGCORE_RESOURCE_ROOT}/*"
        )
        list(APPEND PINGCORE_RESOURCE_INPUTS ${PINGCORE_RESOURCE_ROOT_INPUTS})
    endforeach ()
    list(SORT PINGCORE_RESOURCE_INPUTS)

    _pingcore_get_resource_output(${target_name} PINGCORE_RESOURCE_OUTPUT)
    set_property(TARGET ${target_name} PROPERTY
            PINGCORE_RESOURCE_PACKAGE "${PINGCORE_RESOURCE_OUTPUT}/package"
    )
    set(PINGCORE_RESOURCE_INDEX
            "${PINGCORE_RESOURCE_OUTPUT}/package/pingcore/resources.bin"
    )
    set(PINGCORE_RESOURCE_PLAN
            "${PINGCORE_RESOURCE_OUTPUT}/resource-plan-$<CONFIG>.txt"
    )
    set(PINGCORE_RESOURCE_DRIVER
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PingCoreResourceBuild.cmake"
    )
    string(CONCAT PINGCORE_RESOURCE_PLAN_CONTENT
            "package_inputs=${PINGCORE_RESOURCE_PACKAGE_INPUTS}\n"
            "roots=${PINGCORE_RESOURCE_ROOTS}\n"
            "namespaces=${PINGCORE_RESOURCE_NAMESPACES}\n"
            "inputs=${PINGCORE_RESOURCE_INPUTS}\n"
    )
    file(GENERATE
            OUTPUT "${PINGCORE_RESOURCE_PLAN}"
            CONTENT "${PINGCORE_RESOURCE_PLAN_CONTENT}"
    )
    pingcore_resolve_host_tool("hrc" PINGCORE_RESOURCE_COMPILER_COMMAND)
    add_custom_command(
            OUTPUT "${PINGCORE_RESOURCE_INDEX}"
            COMMAND ${CMAKE_COMMAND}
                    "-DPINGCORE_HRC=${PINGCORE_RESOURCE_COMPILER_COMMAND}"
                    "-DPINGCORE_RESOURCE_PACKAGE_INPUTS=${PINGCORE_RESOURCE_PACKAGE_INPUTS}"
                    "-DPINGCORE_RESOURCE_ROOTS=${PINGCORE_RESOURCE_ROOTS}"
                    "-DPINGCORE_RESOURCE_NAMESPACES=${PINGCORE_RESOURCE_NAMESPACES}"
                    "-DPINGCORE_RESOURCE_OUTPUT=${PINGCORE_RESOURCE_OUTPUT}"
                    -P "${PINGCORE_RESOURCE_DRIVER}"
            DEPENDS
                    ${PINGCORE_RESOURCE_PACKAGE_INDEXES}
                    ${PINGCORE_RESOURCE_INPUTS}
                    "${PINGCORE_RESOURCE_PLAN}"
                    "${PINGCORE_RESOURCE_COMPILER_COMMAND}"
                    "${PINGCORE_RESOURCE_DRIVER}"
            COMMENT "Generating PingCore resources for ${target_name}"
            VERBATIM
    )
    target_include_directories(${target_name} PRIVATE
            "${PINGCORE_RESOURCE_OUTPUT}/include"
    )

    set(PINGCORE_RESOURCE_TARGET_OUTPUT "${PINGCORE_RESOURCE_INDEX}")
    set(PINGCORE_RESOURCE_STAGE_DIRECTORY)
    # Gradle stages Android packages after all ABI builds; CMake stages desktop targets with one output package.
    if (EMSCRIPTEN)
        target_link_options(${target_name} PRIVATE
                "$<$<NOT:$<BOOL:$<TARGET_PROPERTY:_PINGCORE_WEB_TEST>>>:SHELL:--preload-file \"${PINGCORE_RESOURCE_OUTPUT}/package@/\">"
        )
        set_property(TARGET ${target_name} APPEND PROPERTY LINK_DEPENDS
                "${PINGCORE_RESOURCE_INDEX}"
        )
    elseif (APPLE AND NOT IOS)
        get_target_property(PINGCORE_RESOURCE_TARGET_IS_BUNDLE
                ${target_name}
                MACOSX_BUNDLE
        )
        if (PINGCORE_RESOURCE_TARGET_IS_BUNDLE)
            set(PINGCORE_RESOURCE_STAGE_DIRECTORY
                    "$<TARGET_BUNDLE_DIR:${target_name}>/Contents/Resources/PingCore"
            )
        endif ()
    elseif (WIN32 OR (UNIX AND NOT APPLE))
        set(PINGCORE_RESOURCE_STAGE_DIRECTORY
                "$<TARGET_FILE_DIR:${target_name}>/$<TARGET_FILE_BASE_NAME:${target_name}>.resources"
        )
    endif ()

    if (PINGCORE_RESOURCE_STAGE_DIRECTORY)
        set(PINGCORE_RESOURCE_STAGE_STAMP
                "${PINGCORE_RESOURCE_OUTPUT}/stage-$<CONFIG>.stamp"
        )
        add_custom_command(
                OUTPUT "${PINGCORE_RESOURCE_STAGE_STAMP}"
                COMMAND ${CMAKE_COMMAND} -E rm -f
                        "${PINGCORE_RESOURCE_STAGE_STAMP}"
                COMMAND ${CMAKE_COMMAND} -E remove_directory
                        "${PINGCORE_RESOURCE_STAGE_DIRECTORY}"
                COMMAND ${CMAKE_COMMAND} -E make_directory
                        "${PINGCORE_RESOURCE_STAGE_DIRECTORY}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${PINGCORE_RESOURCE_OUTPUT}/package"
                        "${PINGCORE_RESOURCE_STAGE_DIRECTORY}"
                COMMAND ${CMAKE_COMMAND} -E touch
                        "${PINGCORE_RESOURCE_STAGE_STAMP}"
                DEPENDS "${PINGCORE_RESOURCE_INDEX}"
                COMMENT "Staging PingCore resources for ${target_name}"
                VERBATIM
        )
        set(PINGCORE_RESOURCE_TARGET_OUTPUT
                "${PINGCORE_RESOURCE_STAGE_STAMP}"
        )
    endif ()

    set_source_files_properties(
            "${PINGCORE_RESOURCE_TARGET_OUTPUT}"
            PROPERTIES
                    GENERATED TRUE
                    HEADER_FILE_ONLY TRUE
    )
    target_sources(${target_name} PRIVATE
            "${PINGCORE_RESOURCE_TARGET_OUTPUT}"
    )
endfunction()

function(_pingcore_configure_scheduled_resources)
    get_property(PINGCORE_RESOURCE_TARGETS
            DIRECTORY
            PROPERTY PINGCORE_RESOURCE_TARGETS
    )
    foreach (PINGCORE_RESOURCE_TARGET IN LISTS PINGCORE_RESOURCE_TARGETS)
        _pingcore_configure_resources(${PINGCORE_RESOURCE_TARGET})
    endforeach ()
endfunction()

function(_pingcore_schedule_resources target_name)
    get_property(PINGCORE_RESOURCES_ALREADY_SCHEDULED
            TARGET ${target_name}
            PROPERTY PINGCORE_RESOURCES_SCHEDULED
    )
    if (PINGCORE_RESOURCES_ALREADY_SCHEDULED)
        return()
    endif ()

    get_target_property(PINGCORE_RESOURCE_TARGET_SOURCE_DIR ${target_name} SOURCE_DIR)
    set_property(TARGET ${target_name} PROPERTY PINGCORE_RESOURCES_SCHEDULED TRUE)
    set_property(DIRECTORY "${PINGCORE_RESOURCE_TARGET_SOURCE_DIR}" APPEND PROPERTY
            PINGCORE_RESOURCE_TARGETS
            "${target_name}"
    )

    get_property(PINGCORE_RESOURCE_CALLBACK_SCHEDULED
            DIRECTORY "${PINGCORE_RESOURCE_TARGET_SOURCE_DIR}"
            PROPERTY PINGCORE_RESOURCE_CALLBACK_SCHEDULED
    )
    if (NOT PINGCORE_RESOURCE_CALLBACK_SCHEDULED)
        set_property(DIRECTORY "${PINGCORE_RESOURCE_TARGET_SOURCE_DIR}" PROPERTY
                PINGCORE_RESOURCE_CALLBACK_SCHEDULED TRUE
        )
        cmake_language(DEFER
                DIRECTORY "${PINGCORE_RESOURCE_TARGET_SOURCE_DIR}"
                CALL _pingcore_configure_scheduled_resources
        )
    endif ()
endfunction()

function(_pingcore_append_resource_package_input target_name resource_package)
    if (NOT TARGET ${target_name})
        message(FATAL_ERROR "PingCore resource package target does not exist: ${target_name}")
    endif ()
    if (NOT resource_package)
        message(FATAL_ERROR "PingCore resource package input must not be empty")
    endif ()
    set_property(TARGET ${target_name} APPEND PROPERTY
            PINGCORE_RESOURCE_PACKAGE_INPUTS
            "${resource_package}"
    )
    _pingcore_schedule_resources(${target_name})
endfunction()

function(pingcore_add_resources target_name)
    if (NOT TARGET ${target_name})
        message(FATAL_ERROR
                "pingcore_add_resources() target does not exist: ${target_name}"
        )
    endif ()

    cmake_parse_arguments(PINGCORE_RESOURCES
            ""
            "ROOT;NAMESPACE"
            ""
            ${ARGN}
    )
    if (NOT PINGCORE_RESOURCES_ROOT OR NOT PINGCORE_RESOURCES_NAMESPACE)
        message(FATAL_ERROR
                "pingcore_add_resources() requires ROOT and NAMESPACE"
        )
    endif ()
    if (NOT PINGCORE_RESOURCES_NAMESPACE MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR
                "pingcore_add_resources() NAMESPACE must be a C++ identifier"
        )
    endif ()

    file(REAL_PATH
            "${PINGCORE_RESOURCES_ROOT}"
            PINGCORE_RESOURCE_ROOT
            BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    if (NOT IS_DIRECTORY "${PINGCORE_RESOURCE_ROOT}")
        message(FATAL_ERROR
                "pingcore_add_resources() ROOT is not a directory: ${PINGCORE_RESOURCE_ROOT}"
        )
    endif ()

    get_property(PINGCORE_RESOURCE_ROOTS
            TARGET ${target_name}
            PROPERTY PINGCORE_RESOURCE_ROOTS
    )
    foreach (PINGCORE_EXISTING_RESOURCE_ROOT IN LISTS PINGCORE_RESOURCE_ROOTS)
        set(PINGCORE_EXISTING_RESOURCE_ROOT_PATH
                "${PINGCORE_EXISTING_RESOURCE_ROOT}"
        )
        cmake_path(IS_PREFIX
                PINGCORE_EXISTING_RESOURCE_ROOT_PATH
                "${PINGCORE_RESOURCE_ROOT}"
                NORMALIZE
                PINGCORE_EXISTING_RESOURCE_ROOT_IS_PREFIX
        )
        set(PINGCORE_RESOURCE_ROOT_PATH "${PINGCORE_RESOURCE_ROOT}")
        cmake_path(IS_PREFIX
                PINGCORE_RESOURCE_ROOT_PATH
                "${PINGCORE_EXISTING_RESOURCE_ROOT}"
                NORMALIZE
                PINGCORE_RESOURCE_ROOT_IS_PREFIX
        )
        if (PINGCORE_EXISTING_RESOURCE_ROOT_IS_PREFIX
                OR PINGCORE_RESOURCE_ROOT_IS_PREFIX)
            message(FATAL_ERROR
                    "pingcore_add_resources() ROOT overlaps a root already registered for ${target_name}: ${PINGCORE_RESOURCE_ROOT}"
            )
        endif ()
    endforeach ()

    set_property(TARGET ${target_name} APPEND PROPERTY
            PINGCORE_RESOURCE_ROOTS
            "${PINGCORE_RESOURCE_ROOT}"
    )
    set_property(TARGET ${target_name} APPEND PROPERTY
            PINGCORE_RESOURCE_NAMESPACES
            "${PINGCORE_RESOURCES_NAMESPACE}"
    )
    _pingcore_get_resource_output(${target_name} PINGCORE_RESOURCE_OUTPUT)
    set_property(TARGET ${target_name} PROPERTY
            PINGCORE_RESOURCE_PACKAGE
            "${PINGCORE_RESOURCE_OUTPUT}/package"
    )
    _pingcore_schedule_resources(${target_name})
endfunction()

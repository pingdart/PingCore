include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/PingCoreLibraries.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/PingCoreRuntimeDependencies.cmake")

function(_pingcore_configure_ios_app_core target_name)
    get_property(PINGCORE_IOS_APP_FRAMEWORK_TARGET
            TARGET ${target_name}
            PROPERTY PINGCORE_APP_FRAMEWORK_TARGET
    )
    set(PINGCORE_IOS_LIBRARY_TARGETS "")
    set(PINGCORE_IOS_REQUESTED_LIBRARY_TARGETS "")
    _pingcore_collect_library_dependencies("${target_name}" ""
            PINGCORE_IOS_LIBRARY_TARGETS PINGCORE_IOS_REQUESTED_LIBRARY_TARGETS
    )
    find_program(PINGCORE_IOS_LIBTOOL libtool REQUIRED)
    set(PINGCORE_IOS_CORE_DIRECTORY
            "${CMAKE_BINARY_DIR}/pingcore-ios/${target_name}"
    )
    set(PINGCORE_IOS_CORE_ARCHIVE
            "${PINGCORE_IOS_CORE_DIRECTORY}/lib${target_name}_pingcore.a"
    )
    set(PINGCORE_IOS_LINK_OPTIONS_FILE
            "${PINGCORE_IOS_CORE_DIRECTORY}/link.rsp"
    )
    get_target_property(PINGCORE_IOS_LINK_LIBRARIES
            ${PINGCORE_IOS_APP_FRAMEWORK_TARGET}
            INTERFACE_LINK_LIBRARIES
    )
    get_target_property(PINGCORE_IOS_LINK_OPTIONS
            ${PINGCORE_IOS_APP_FRAMEWORK_TARGET}
            INTERFACE_LINK_OPTIONS
    )
    if (NOT PINGCORE_IOS_LINK_LIBRARIES
            OR PINGCORE_IOS_LINK_LIBRARIES MATCHES "-NOTFOUND$")
        message(FATAL_ERROR
                "pingcore_add_app() requires iOS platform link arguments"
        )
    endif ()
    if (PINGCORE_IOS_LINK_OPTIONS MATCHES "-NOTFOUND$")
        set(PINGCORE_IOS_LINK_OPTIONS)
    endif ()
    set(PINGCORE_IOS_LINK_OPTIONS_CONTENT
            "-force_load\n\"${PINGCORE_IOS_CORE_ARCHIVE}\"\n"
    )
    foreach (PINGCORE_IOS_LINK_ARGUMENT IN LISTS
            PINGCORE_IOS_LINK_LIBRARIES
            PINGCORE_IOS_LINK_OPTIONS
    )
        string(APPEND PINGCORE_IOS_LINK_OPTIONS_CONTENT
                "${PINGCORE_IOS_LINK_ARGUMENT}\n"
        )
    endforeach ()
    file(MAKE_DIRECTORY "${PINGCORE_IOS_CORE_DIRECTORY}")
    file(WRITE "${PINGCORE_IOS_LINK_OPTIONS_FILE}"
            "${PINGCORE_IOS_LINK_OPTIONS_CONTENT}"
    )

    set(PINGCORE_IOS_CORE_INPUTS
            "$<TARGET_FILE:${target_name}>"
            "$<TARGET_FILE:${PINGCORE_IOS_APP_FRAMEWORK_TARGET}>"
    )
    set(PINGCORE_IOS_CORE_DEPENDENCIES
            ${target_name}
            ${PINGCORE_IOS_APP_FRAMEWORK_TARGET}
    )
    foreach (PINGCORE_IOS_LIBRARY_TARGET IN LISTS PINGCORE_IOS_LIBRARY_TARGETS)
        list(APPEND PINGCORE_IOS_CORE_INPUTS
                "$<TARGET_FILE:${PINGCORE_IOS_LIBRARY_TARGET}>"
        )
        list(APPEND PINGCORE_IOS_CORE_DEPENDENCIES
                ${PINGCORE_IOS_LIBRARY_TARGET}
        )
    endforeach ()

    add_custom_command(
            OUTPUT "${PINGCORE_IOS_CORE_ARCHIVE}"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${PINGCORE_IOS_CORE_DIRECTORY}"
            COMMAND "${PINGCORE_IOS_LIBTOOL}" -static
                    -o "${PINGCORE_IOS_CORE_ARCHIVE}"
                    ${PINGCORE_IOS_CORE_INPUTS}
            DEPENDS ${PINGCORE_IOS_CORE_DEPENDENCIES}
            COMMENT "Linking PingCore iOS application core ${target_name}"
            VERBATIM
    )
    add_custom_target(${target_name}_pingcore_ios_core
            DEPENDS "${PINGCORE_IOS_CORE_ARCHIVE}"
    )
endfunction()

function(_pingcore_configure_scheduled_ios_app_cores)
    get_property(PINGCORE_IOS_APP_TARGETS
            DIRECTORY
            PROPERTY PINGCORE_IOS_APP_TARGETS
    )
    foreach (PINGCORE_IOS_APP_TARGET IN LISTS PINGCORE_IOS_APP_TARGETS)
        _pingcore_configure_ios_app_core(${PINGCORE_IOS_APP_TARGET})
    endforeach ()
endfunction()

function(_pingcore_schedule_ios_app_core target_name)
    get_target_property(PINGCORE_IOS_APP_SOURCE_DIRECTORY
            ${target_name}
            SOURCE_DIR
    )
    set_property(DIRECTORY "${PINGCORE_IOS_APP_SOURCE_DIRECTORY}" APPEND PROPERTY
            PINGCORE_IOS_APP_TARGETS
            "${target_name}"
    )
    get_property(PINGCORE_IOS_APP_CALLBACK_SCHEDULED
            DIRECTORY "${PINGCORE_IOS_APP_SOURCE_DIRECTORY}"
            PROPERTY PINGCORE_IOS_APP_CALLBACK_SCHEDULED
    )
    if (NOT PINGCORE_IOS_APP_CALLBACK_SCHEDULED)
        set_property(DIRECTORY "${PINGCORE_IOS_APP_SOURCE_DIRECTORY}" PROPERTY
                PINGCORE_IOS_APP_CALLBACK_SCHEDULED TRUE
        )
        cmake_language(DEFER
                DIRECTORY "${PINGCORE_IOS_APP_SOURCE_DIRECTORY}"
                CALL _pingcore_configure_scheduled_ios_app_cores
        )
    endif ()
endfunction()

function(pingcore_add_app target_name)
    cmake_parse_arguments(PINGCORE_APP
            ""
            "RESOURCE_NAMESPACE;RESOURCE_OUTPUT_DIRECTORY;BUNDLE_NAME;BUNDLE_IDENTIFIER"
            "SOURCES;RESOURCES"
            ${ARGN}
    )

    if (TARGET ${target_name})
        message(FATAL_ERROR
                "pingcore_add_app() target already exists: ${target_name}"
        )
    endif ()
    if (NOT PINGCORE_APP_SOURCES)
        message(FATAL_ERROR
                "pingcore_add_app() requires at least one source"
        )
    endif ()
    if (PINGCORE_APP_RESOURCES AND NOT PINGCORE_APP_RESOURCE_NAMESPACE)
        message(FATAL_ERROR
                "pingcore_add_app() requires RESOURCE_NAMESPACE when RESOURCES is present"
        )
    endif ()
    list(LENGTH PINGCORE_APP_RESOURCES PINGCORE_APP_RESOURCE_ROOT_COUNT)
    if (PINGCORE_APP_RESOURCE_ROOT_COUNT GREATER 1)
        message(FATAL_ERROR
                "pingcore_add_app() currently accepts one resource root"
        )
    endif ()
    set(PINGCORE_APP_INSTALL_COMPONENT "PingCoreApplication")

    if (PINGCORE_LIBRARY_GRAPH_ONLY)
        if (NOT PINGCORE_LIBRARY_GRAPH_OUTPUT)
            message(FATAL_ERROR
                    "PINGCORE_LIBRARY_GRAPH_ONLY requires PINGCORE_LIBRARY_GRAPH_OUTPUT"
            )
        endif ()
        add_library(${target_name} INTERFACE)
        set_property(TARGET ${target_name} PROPERTY
                PINGCORE_LIBRARY_GRAPH_ONLY TRUE
        )
        set_property(TARGET ${target_name} PROPERTY
                PINGCORE_APPLICATION_INSTALL_COMPONENT
                "${PINGCORE_APP_INSTALL_COMPONENT}"
        )
        get_filename_component(PINGCORE_APP_LIBRARY_GRAPH_OUTPUT
                "${PINGCORE_LIBRARY_GRAPH_OUTPUT}"
                ABSOLUTE
                BASE_DIR "${CMAKE_BINARY_DIR}"
        )
        set_property(TARGET ${target_name} PROPERTY
                PINGCORE_LIBRARY_GRAPH_OUTPUT
                "${PINGCORE_APP_LIBRARY_GRAPH_OUTPUT}"
        )
        cmake_language(EVAL CODE
                "cmake_language(DEFER CALL _pingcore_write_library_graph [[${target_name}]])"
        )
        return()
    endif ()

    _pingcore_select_framework_target(PINGCORE_APP_FRAMEWORK_TARGET)
    get_target_property(PINGCORE_APP_FRAMEWORK_RESOURCE_PACKAGE
            ${PINGCORE_APP_FRAMEWORK_TARGET}
            PINGCORE_RESOURCE_PACKAGE
    )
    if (NOT PINGCORE_APP_FRAMEWORK_RESOURCE_PACKAGE
            OR PINGCORE_APP_FRAMEWORK_RESOURCE_PACKAGE MATCHES "-NOTFOUND$")
        message(FATAL_ERROR "pingcore_add_app() requires the PingCore built-in resource package")
    endif ()

    if (IOS)
        add_library(${target_name} STATIC ${PINGCORE_APP_SOURCES})
        set_target_properties(${target_name} PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        )
    elseif (ANDROID)
        add_library(${target_name} SHARED ${PINGCORE_APP_SOURCES})
    else ()
        add_executable(${target_name} ${PINGCORE_APP_SOURCES})
    endif ()

    if (WIN32)
        set_target_properties(${target_name} PROPERTIES WIN32_EXECUTABLE TRUE)
        if (MSVC)
            # Preserve CRT initialization and the portable main() entry.
            set_property(TARGET ${target_name} PROPERTY PINGCORE_WINDOWS_CRT_ENTRY mainCRTStartup)
            target_link_options(${target_name} PRIVATE
                    "LINKER:/ENTRY:$<TARGET_PROPERTY:${target_name},PINGCORE_WINDOWS_CRT_ENTRY>"
            )
        endif ()
    endif ()

    set_property(TARGET ${target_name} PROPERTY
            PINGCORE_APPLICATION_INSTALL_COMPONENT
            "${PINGCORE_APP_INSTALL_COMPONENT}"
    )

    target_compile_features(${target_name} PRIVATE cxx_std_20)
    target_compile_options(${target_name} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    _pingcore_configure_optimization(${target_name})
    target_link_libraries(${target_name} PRIVATE
            ${PINGCORE_APP_FRAMEWORK_TARGET}
    )

    if (PINGCORE_APP_RESOURCE_OUTPUT_DIRECTORY)
        get_filename_component(PINGCORE_APP_RESOURCE_OUTPUT_DIRECTORY
                "${PINGCORE_APP_RESOURCE_OUTPUT_DIRECTORY}"
                ABSOLUTE
                BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}"
        )
        set_property(TARGET ${target_name} PROPERTY
                PINGCORE_RESOURCE_OUTPUT_DIRECTORY
                "${PINGCORE_APP_RESOURCE_OUTPUT_DIRECTORY}"
        )
    endif ()

    if (IOS)
        set_property(TARGET ${target_name} PROPERTY
                PINGCORE_RESOURCE_OUTPUT_DIRECTORY
                "${CMAKE_BINARY_DIR}/pingcore-ios/${target_name}/resources"
        )
    endif ()

    if (APPLE AND NOT IOS)
        set_target_properties(${target_name} PROPERTIES MACOSX_BUNDLE TRUE)
        if (PINGCORE_APP_BUNDLE_NAME)
            set_target_properties(${target_name} PROPERTIES
                    MACOSX_BUNDLE_BUNDLE_NAME "${PINGCORE_APP_BUNDLE_NAME}"
            )
        endif ()
        if (PINGCORE_APP_BUNDLE_IDENTIFIER)
            set_target_properties(${target_name} PROPERTIES
                    MACOSX_BUNDLE_GUI_IDENTIFIER "${PINGCORE_APP_BUNDLE_IDENTIFIER}"
            )
        endif ()
    endif ()

    pingcore_enable_codegen(${target_name})
    _pingcore_append_resource_package_input(
            ${target_name}
            "${PINGCORE_APP_FRAMEWORK_RESOURCE_PACKAGE}"
    )
    if (PINGCORE_APP_RESOURCES)
        list(GET PINGCORE_APP_RESOURCES 0 PINGCORE_APP_RESOURCE_ROOT)
        pingcore_add_resources(${target_name}
                ROOT "${PINGCORE_APP_RESOURCE_ROOT}"
                NAMESPACE "${PINGCORE_APP_RESOURCE_NAMESPACE}"
        )
    endif ()

    if (IOS)
        set_property(TARGET ${target_name} PROPERTY
                PINGCORE_APP_FRAMEWORK_TARGET
                "${PINGCORE_APP_FRAMEWORK_TARGET}"
        )
        _pingcore_schedule_ios_app_core(${target_name})
        return()
    endif ()

    _pingcore_json_escape("${target_name}" PINGCORE_APP_JSON_TARGET)
    _pingcore_json_escape("${PINGCORE_APP_BUNDLE_IDENTIFIER}" PINGCORE_APP_JSON_BUNDLE_IDENTIFIER)
    _pingcore_json_escape("${PINGCORE_APP_BUNDLE_NAME}" PINGCORE_APP_JSON_BUNDLE_NAME)

    set(PINGCORE_APP_INTEGRATION_DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}/pingcore-integration/${target_name}"
    )
    set(PINGCORE_APP_INTEGRATION_PLAN
            "${PINGCORE_APP_INTEGRATION_DIRECTORY}/$<CONFIG>/app.json"
    )
    if (ANDROID AND PINGCORE_ANDROID_APP_INTEGRATION_ROOT)
        # Gradle supplies a variant-specific root; each NDK configure owns exactly one ABI and configuration.
        set(PINGCORE_APP_INTEGRATION_DIRECTORY "${PINGCORE_ANDROID_APP_INTEGRATION_ROOT}/${ANDROID_ABI}")
        set(PINGCORE_APP_INTEGRATION_PLAN "${PINGCORE_APP_INTEGRATION_DIRECTORY}/app.json")
    endif ()
    set(PINGCORE_APP_BUNDLE_PATH)
    if (APPLE)
        set(PINGCORE_APP_BUNDLE_PATH "$<TARGET_BUNDLE_DIR:${target_name}>")
    endif ()
    file(MAKE_DIRECTORY "${PINGCORE_APP_INTEGRATION_DIRECTORY}")
    file(GENERATE
            OUTPUT "${PINGCORE_APP_INTEGRATION_PLAN}"
            CONTENT "{\n  \"schema\": 1,\n  \"target\": \"${PINGCORE_APP_JSON_TARGET}\",\n  \"name\": \"${PINGCORE_APP_JSON_BUNDLE_NAME}\",\n  \"version\": \"${PROJECT_VERSION}\",\n  \"artifact\": \"$<TARGET_FILE:${target_name}>\",\n  \"bundle\": \"${PINGCORE_APP_BUNDLE_PATH}\",\n  \"bundleIdentifier\": \"${PINGCORE_APP_JSON_BUNDLE_IDENTIFIER}\",\n  \"installComponent\": \"${PINGCORE_APP_INSTALL_COMPONENT}\"\n}\n"
    )
endfunction()

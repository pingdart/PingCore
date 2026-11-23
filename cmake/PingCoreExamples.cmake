include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/PingCoreApp.cmake")

function(pingcore_add_example target_name bundle_name bundle_identifier)
    set(PINGCORE_EXAMPLE_SOURCES main.cpp)
    if (NOT IOS AND NOT ANDROID AND NOT EMSCRIPTEN)
        list(APPEND PINGCORE_EXAMPLE_SOURCES
                "${PINGCORE_PROJECT_DIR}/examples/main.cpp"
        )
    endif ()

    set(PINGCORE_EXAMPLE_RESOURCE_OUTPUT_ARGUMENTS)
    if (ANDROID AND PINGCORE_ANDROID_RESOURCE_OUTPUT_ROOT)
        list(APPEND PINGCORE_EXAMPLE_RESOURCE_OUTPUT_ARGUMENTS
                RESOURCE_OUTPUT_DIRECTORY
                "${PINGCORE_ANDROID_RESOURCE_OUTPUT_ROOT}/${ANDROID_ABI}"
        )
    endif ()

    pingcore_add_app(${target_name}
            SOURCES ${PINGCORE_EXAMPLE_SOURCES}
            BUNDLE_NAME "${bundle_name}"
            BUNDLE_IDENTIFIER "${bundle_identifier}"
            ${PINGCORE_EXAMPLE_RESOURCE_OUTPUT_ARGUMENTS}
    )

    if (EMSCRIPTEN)
        set_target_properties(${target_name} PROPERTIES SUFFIX ".js")
        set(PINGCORE_WEB_MODULE_FILE "${target_name}.js")
        set(PINGCORE_WEB_STORAGE_KEY "${bundle_identifier}")
        configure_file(
                "${PINGCORE_PROJECT_DIR}/platform/web/example.html.in"
                "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.html"
                @ONLY
        )
        get_target_property(PINGCORE_WEB_OUTPUT_DIRECTORY ${target_name} RUNTIME_OUTPUT_DIRECTORY)
        if (NOT PINGCORE_WEB_OUTPUT_DIRECTORY)
            set(PINGCORE_WEB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
        endif ()
        set(PINGCORE_WEB_ENTRY_FILE "${PINGCORE_WEB_OUTPUT_DIRECTORY}/${target_name}.html")
        add_custom_command(OUTPUT "${PINGCORE_WEB_ENTRY_FILE}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${PINGCORE_WEB_OUTPUT_DIRECTORY}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.html"
                        "${PINGCORE_WEB_ENTRY_FILE}"
                DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${target_name}.html"
                VERBATIM
        )
        add_custom_target(${target_name}_web_entry DEPENDS "${PINGCORE_WEB_ENTRY_FILE}")
        add_dependencies(${target_name} ${target_name}_web_entry)
    endif ()
endfunction()

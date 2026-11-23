function(pingcore_configure_web_app target_name)
    if (NOT TARGET ${target_name})
        message(FATAL_ERROR "pingcore_configure_web_app() target does not exist: ${target_name}")
    endif ()

    set_target_properties(${target_name} PROPERTIES SUFFIX ".js")
    set(PINGCORE_WEB_MODULE_FILE "${target_name}.js")
    set(PINGCORE_WEB_STORAGE_KEY "@PROJECT_ID@")
    set(PINGCORE_WEB_ENTRY_FILE "${target_name}.html")
    set(PINGCORE_WEB_FAVICON_FILE "${target_name}.favicon.svg")
    set(PINGCORE_WEB_APPLE_TOUCH_ICON_FILE "${target_name}.apple-touch-icon.png")
    set(PINGCORE_WEB_GENERATED_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/pingcore-web")
    file(MAKE_DIRECTORY "${PINGCORE_WEB_GENERATED_DIRECTORY}")
    configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/index.html.in"
            "${PINGCORE_WEB_GENERATED_DIRECTORY}/${target_name}.html"
            @ONLY
    )
    configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/favicon.svg"
            "${PINGCORE_WEB_GENERATED_DIRECTORY}/${PINGCORE_WEB_FAVICON_FILE}"
            COPYONLY
    )
    configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/apple-touch-icon.png"
            "${PINGCORE_WEB_GENERATED_DIRECTORY}/${PINGCORE_WEB_APPLE_TOUCH_ICON_FILE}"
            COPYONLY
    )
    # Shell-only edits and deleted output files must not require relinking the application.
    add_custom_target(${target_name}_web_assets
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${PINGCORE_WEB_GENERATED_DIRECTORY}/${target_name}.html"
                    "${PINGCORE_WEB_GENERATED_DIRECTORY}/${PINGCORE_WEB_FAVICON_FILE}"
                    "${PINGCORE_WEB_GENERATED_DIRECTORY}/${PINGCORE_WEB_APPLE_TOUCH_ICON_FILE}"
                    "$<TARGET_FILE_DIR:${target_name}>"
            VERBATIM
    )
    add_dependencies(${target_name} ${target_name}_web_assets)
endfunction()

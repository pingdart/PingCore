set(PINGCORE_PLATFORM_SOURCE_FILES
        "${PINGCORE_PROJECT_DIR}/platform/web/web_adapter.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/web/web_application.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/web/web_external_texture.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/web/web_file.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/web/web_file_picker.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/web/web_http.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/web/web_navigation.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/web/web_platform_registry.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/web/web_platform_view.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/web/web_renderer.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/web/web_text_input.cpp"
)
set(PINGCORE_PLATFORM_COMPILE_OPTIONS
        -fexceptions
)
set(PINGCORE_PLATFORM_INTERFACE_COMPILE_OPTIONS
        -fexceptions
)
# Repository tests supply their own Node.js executable options on the consuming target.
set(web_application "$<NOT:$<BOOL:$<TARGET_PROPERTY:_PINGCORE_WEB_TEST>>>")
set(PINGCORE_PLATFORM_LINK_OPTIONS
        --bind
        "$<${web_application}:--no-entry>"
        "$<BUILD_INTERFACE:SHELL:--pre-js \"${PINGCORE_PROJECT_DIR}/platform/web/web_platform_registry.js\">"
        "$<BUILD_INTERFACE:$<${web_application}:SHELL:--pre-js \"${PINGCORE_PROJECT_DIR}/platform/web/web_file.js\">>"
        -fexceptions
        -lidbfs.js
        "$<${web_application}:-sMODULARIZE=1>"
        "$<${web_application}:-sEXPORT_ES6=1>"
        "$<${web_application}:-sEXPORT_NAME=createPingCoreApp>"
        "-sEXPORTED_RUNTIME_METHODS=stringToNewUTF8"
        "$<${web_application}:-sENVIRONMENT=web>"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sSTACK_SIZE=1048576"
        "$<${web_application}:-sNO_EXIT_RUNTIME=1>"
)

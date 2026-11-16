# cmake/PingDartWeb.cmake
# PingDart Framework - Web & WebAssembly Macro Helpers

function(add_pingdart_web_app TARGET_NAME)
    cmake_parse_arguments(ARG "" "HTML_TEMPLATE;OUTPUT_NAME" "SOURCES;DEPENDS" ${ARGN})

    add_executable(${TARGET_NAME} ${ARG_SOURCES})

    target_link_libraries(${TARGET_NAME} PRIVATE
        PingDart::Web
        PingDart::Core
        ${ARG_DEPENDS}
    )

    if(EMSCRIPTEN)
        set_target_properties(${TARGET_NAME} PROPERTIES
            OUTPUT_NAME "${ARG_OUTPUT_NAME}"
            SUFFIX ".js"
        )
        if(ARG_HTML_TEMPLATE)
            target_link_options(${TARGET_NAME} PRIVATE "--shell-file" "${ARG_HTML_TEMPLATE}")
        endif()
    endif()
endfunction()

set(PINGCORE_PUBLIC_HEADERS
        testing/ui_test.h
        animation.h
        app.h
        bitmap.h
        clipboard.h
        color.h
        data.h
        environment.h
        event.h
        external_texture.h
        file.h
        file_drop.h
        geometry.h
        gesture.h
        http.h
        pingcore.h
        indication.h
        layer.h
        layout.h
        lifecycle.h
        modifier.h
        navigation.h
        paint.h
        platform_registry.h
        presentation.h
        render_scene.h
        resource.h
        scroll.h
        semantics.h
        state.h
        stream.h
        system.h
        system_task.h
        task.h
        text.h
        text_input.h
        theme.h
        validation.h
        vector.h
        view.h
        virtual_layout.h
        window.h
)

set(PINGCORE_ANDROID_PUBLIC_HEADERS
        android/external_texture.h
        android/jni.h
        android/platform_registry.h
)

set(PINGCORE_IOS_PUBLIC_HEADERS
        ios/platform_registry.h
)

set(PINGCORE_MACOS_PUBLIC_HEADERS
        macos/platform_registry.h
)

if (WIN32)
    list(APPEND PINGCORE_WINDOWS_PUBLIC_HEADERS
            windows/external_texture.h
            windows/installer.h
            windows/platform_registry.h
    )
endif ()

if (APPLE)
    list(APPEND PINGCORE_IOS_PUBLIC_HEADERS ios/external_texture.h)
    list(APPEND PINGCORE_MACOS_PUBLIC_HEADERS macos/external_texture.h)
endif ()

if (EMSCRIPTEN)
    list(APPEND PINGCORE_WEB_PUBLIC_HEADERS
            web/external_texture.h
            web/navigation.h
            web/platform_registry.h
    )
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND PINGCORE_LINUX_PUBLIC_HEADERS
            linux/external_texture.h
            linux/platform_registry.h
    )
endif ()

function(pingcore_add_header_checks target_name)
    set(PINGCORE_HEADER_TEST_SOURCES)
    foreach (PINGCORE_PUBLIC_HEADER IN LISTS ARGN)
        string(REPLACE "." "_" PINGCORE_HEADER_TEST_NAME
                "${PINGCORE_PUBLIC_HEADER}"
        )
        string(REPLACE "/" "_" PINGCORE_HEADER_TEST_NAME
                "${PINGCORE_HEADER_TEST_NAME}"
        )
        set(PINGCORE_HEADER_TEST_SOURCE
                "${CMAKE_CURRENT_BINARY_DIR}/header_compile/${target_name}/${PINGCORE_HEADER_TEST_NAME}.cpp"
        )
        file(GENERATE
                OUTPUT "${PINGCORE_HEADER_TEST_SOURCE}"
                CONTENT "#include <pingcore/${PINGCORE_PUBLIC_HEADER}>\n"
        )
        list(APPEND PINGCORE_HEADER_TEST_SOURCES
                "${PINGCORE_HEADER_TEST_SOURCE}"
        )
    endforeach ()

    add_library(${target_name} OBJECT
            ${PINGCORE_HEADER_TEST_SOURCES}
    )

    target_link_libraries(
            ${target_name}
            PRIVATE ${PINGCORE_TEST_LIBRARY}
    )

endfunction()

pingcore_add_header_checks(pingcore_header_checks ${PINGCORE_PUBLIC_HEADERS})
if (PINGCORE_WINDOWS_PUBLIC_HEADERS)
    pingcore_add_header_checks(pingcore_windows_header_checks ${PINGCORE_WINDOWS_PUBLIC_HEADERS})
    target_sources(pingcore_windows_header_checks PRIVATE
            "${PINGCORE_TEST_DIR}/platform/windows/windows_before_pingcore.cpp"
            "${PINGCORE_TEST_DIR}/platform/windows/pingcore_before_windows.cpp"
    )
    target_compile_definitions(pingcore_windows_header_checks PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif ()
if (PINGCORE_LINUX_PUBLIC_HEADERS)
    pingcore_add_header_checks(pingcore_linux_header_checks ${PINGCORE_LINUX_PUBLIC_HEADERS})
endif ()
if (PINGCORE_MACOS_PUBLIC_HEADERS)
    pingcore_add_header_checks(pingcore_macos_header_checks ${PINGCORE_MACOS_PUBLIC_HEADERS})
endif ()
if (PINGCORE_IOS_PUBLIC_HEADERS)
    pingcore_add_header_checks(pingcore_ios_header_checks ${PINGCORE_IOS_PUBLIC_HEADERS})
endif ()
if (PINGCORE_ANDROID_PUBLIC_HEADERS)
    pingcore_add_header_checks(pingcore_android_header_checks ${PINGCORE_ANDROID_PUBLIC_HEADERS})
endif ()
if (PINGCORE_WEB_PUBLIC_HEADERS)
    pingcore_add_header_checks(pingcore_web_header_checks ${PINGCORE_WEB_PUBLIC_HEADERS})
endif ()

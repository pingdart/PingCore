if (PINGCORE_WINDOWS_7_COMPAT)
    set(PINGCORE_WINDOWS_VERSION_DEFINITIONS
            PINGCORE_WINDOWS_7_COMPAT=1
            WINVER=0x0601
            _WIN32_WINNT=0x0601
    )
else ()
    set(PINGCORE_WINDOWS_VERSION_DEFINITIONS
            WINVER=0x0A00
            _WIN32_WINNT=0x0A00
    )
endif ()

set(PINGCORE_PLATFORM_SOURCE_FILES
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_adapter.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_accessibility.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_application.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_external_texture.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_file.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_file_drop.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_file_picker.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_http.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_platform_view.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_renderer.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_system_tray.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_text_input.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/windows/win32_ui_dispatcher.cpp"
)
set(PINGCORE_PLATFORM_COMPILE_DEFINITIONS
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        ${PINGCORE_WINDOWS_VERSION_DEFINITIONS}
)
set(PINGCORE_PLATFORM_LINK_LIBRARIES
        advapi32
        d2d1
        d3d11
        dwrite
        dwmapi
        dxguid
        dxgi
        imm32
        ole32
        oleaut32
        psapi
        shell32
        uiautomationcore
        user32
        winhttp
        windowscodecs
)
if (NOT PINGCORE_WINDOWS_7_COMPAT)
    list(APPEND PINGCORE_PLATFORM_LINK_LIBRARIES dcomp)
    list(APPEND PINGCORE_PLATFORM_LINK_LIBRARIES windowsapp)
    list(APPEND PINGCORE_PLATFORM_LINK_LIBRARIES crypt32 propsys)
endif ()

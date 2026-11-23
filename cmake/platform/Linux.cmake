find_package(PkgConfig REQUIRED)

# GTK owns the native display connection, X11/Wayland selection, event loop,
# input method integration, clipboard, and presentation
# surface. Pango and Cairo arrive through GTK's public pkg-config closure.
pkg_check_modules(PINGCORE_GTK4 REQUIRED IMPORTED_TARGET gtk4>=4.14)
pkg_check_modules(PINGCORE_EPOXY REQUIRED IMPORTED_TARGET epoxy>=1.5)
pkg_check_modules(PINGCORE_GIO REQUIRED IMPORTED_TARGET gio-2.0)
pkg_check_modules(PINGCORE_FONTS REQUIRED IMPORTED_TARGET pangoft2 fontconfig)
pkg_check_modules(PINGCORE_LIBSOUP QUIET IMPORTED_TARGET libsoup-3.0>=3.0)
if (NOT TARGET PkgConfig::PINGCORE_LIBSOUP)
    message(FATAL_ERROR
            "PingCore Linux HTTP requires the system libsoup 3 development package (minimum 3.0).")
endif ()

set(PINGCORE_PLATFORM_SOURCE_FILES
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_adapter.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_external_texture.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_file.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_file_picker.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_file_drop.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_http.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_platform_view.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_renderer.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_system_tray.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_text_input.cpp"
        "${PINGCORE_PROJECT_DIR}/platform/linux/linux_ui_dispatcher.cpp"
)
set(PINGCORE_PLATFORM_INCLUDE_DIRECTORIES
        ${PINGCORE_GTK4_INCLUDE_DIRS}
        ${PINGCORE_EPOXY_INCLUDE_DIRS}
        ${PINGCORE_GIO_INCLUDE_DIRS}
        ${PINGCORE_FONTS_INCLUDE_DIRS}
        ${PINGCORE_LIBSOUP_INCLUDE_DIRS}
)
set(PINGCORE_PLATFORM_COMPILE_OPTIONS
        ${PINGCORE_GTK4_CFLAGS_OTHER}
        ${PINGCORE_EPOXY_CFLAGS_OTHER}
        ${PINGCORE_GIO_CFLAGS_OTHER}
        ${PINGCORE_FONTS_CFLAGS_OTHER}
        ${PINGCORE_LIBSOUP_CFLAGS_OTHER}
)
set(PINGCORE_PLATFORM_LINK_LIBRARIES
        PkgConfig::PINGCORE_GTK4
        PkgConfig::PINGCORE_EPOXY
        PkgConfig::PINGCORE_GIO
        PkgConfig::PINGCORE_FONTS
        PkgConfig::PINGCORE_LIBSOUP
)

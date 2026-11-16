# src/pal/linux/CMakeLists.txt
# Linux Wayland / X11 Platform Abstraction Layer (ENG-303/304)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(WAYLAND_CLIENT QUIET wayland-client)
    pkg_check_modules(XKBCOMMON QUIET xkbcommon)
    pkg_check_modules(X11 QUIET x11)
endif()

add_library(pingdart_pal_linux STATIC
    wayland/wayland_registry.cpp
    wayland/wayland_event_pump.cpp
    wayland/wayland_input_dispatcher.cpp
    wayland/wayland_window.cpp
    wayland/wayland_platform_host.cpp
    x11/x11_event_pump.cpp
    x11/x11_input_dispatcher.cpp
    x11/x11_window.cpp
    x11/x11_platform_host.cpp
    linux_platform_host_factory.cpp
)

target_include_directories(pingdart_pal_linux
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/wayland
        ${CMAKE_CURRENT_SOURCE_DIR}/x11
)

target_link_libraries(pingdart_pal_linux
    PUBLIC
        PingDart::Core
        PingDart::PAL
)

if(WAYLAND_CLIENT_FOUND)
    target_include_directories(pingdart_pal_linux PUBLIC ${WAYLAND_CLIENT_INCLUDE_DIRS})
    target_link_libraries(pingdart_pal_linux PUBLIC ${WAYLAND_CLIENT_LIBRARIES})
endif()

if(XKBCOMMON_FOUND)
    target_include_directories(pingdart_pal_linux PUBLIC ${XKBCOMMON_INCLUDE_DIRS})
    target_link_libraries(pingdart_pal_linux PUBLIC ${XKBCOMMON_LIBRARIES})
endif()

if(X11_FOUND)
    target_include_directories(pingdart_pal_linux PUBLIC ${X11_INCLUDE_DIRS})
    target_link_libraries(pingdart_pal_linux PUBLIC ${X11_LIBRARIES})
endif()

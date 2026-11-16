# src/pal/windows/CMakeLists.txt
# Windows Win32 Platform Abstraction Layer (ENG-302)

add_library(pingdart_pal_windows STATIC
    win32_event_pump.cpp
    win32_input_dispatcher.cpp
    win32_window.cpp
    win32_platform_host.cpp
)

target_include_directories(pingdart_pal_windows
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pingdart_pal_windows
    PUBLIC
        PingDart::Core
        PingDart::PAL
        user32
        gdi32
        dwmapi
        shcore
        imm32
)

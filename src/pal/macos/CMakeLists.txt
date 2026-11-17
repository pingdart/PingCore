# src/pal/macos/CMakeLists.txt
# PKG-113: Native macOS Platform Adapter Library

add_library(pingdart_pal_macos STATIC
    macos_platform_host.mm
    macos_window.mm
    macos_event_pump.mm
    macos_input_dispatcher.mm
)

target_include_directories(pingdart_pal_macos
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(pingdart_pal_macos PRIVATE -fobjc-arc)

target_link_libraries(pingdart_pal_macos
    PUBLIC
        PingDart::Core
        PingDart::PAL
        "-framework Cocoa"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Carbon"
)

add_library(PingDart::PALMacos ALIAS pingdart_pal_macos)

# src/rhi/CMakeLists.txt

add_library(pingdart_rhi STATIC
    render_device.cpp
    mock/mock_render_device.cpp
    display_list.cpp
    display_list_mailbox.cpp
    compositor.cpp
    vulkan/vulkan_dispatcher.cpp
    vulkan/vulkan_dispatcher_dynamic.cpp
    vulkan/vulkan_glyph_pipeline.cpp
    vulkan/vulkan_surface.cpp
    vulkan/vulkan_texture.cpp
    vulkan/vulkan_command_buffer.cpp
    vulkan/vulkan_swapchain.cpp
    vulkan/vulkan_device.cpp
)

target_include_directories(pingdart_rhi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pingdart_rhi
    PUBLIC
        PingDart::Core
        PingDart::PAL
)

add_library(PingDart::RHI ALIAS pingdart_rhi)

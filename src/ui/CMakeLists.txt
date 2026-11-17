add_library(pingdart_ui STATIC
    render_object.cpp
    widget.cpp
    element.cpp
    widget_tree.cpp
    layout_primitives.cpp
    leaf_widgets.cpp
    focus_node.cpp
    text_editing_controller.cpp
    input_dispatcher.cpp
)

target_include_directories(pingdart_ui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(pingdart_ui
    PUBLIC
        PingDart::Core
        PingDart::PAL
        PingDart::RHI
)

if(TARGET PingDart::Text)
    target_link_libraries(pingdart_ui PUBLIC PingDart::Text)
endif()

add_library(PingDart::UI ALIAS pingdart_ui)

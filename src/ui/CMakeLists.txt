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
    runtime/reconciler.cpp
    runtime/scheduler.cpp
    runtime/component_instance.cpp
    runtime/application.cpp
    events/event.cpp
    events/dispatcher.cpp
    gestures/gesture.cpp
    adapters/qt/qt_runtime_adapter.cpp
    adapters/web/web_runtime_adapter.cpp
    adapters/native/rhi_runtime_adapter.cpp
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

# PKG-138: Optional Qt 6.5+ system-appearance adapter (system_theme.h)
option(
    PINGDART_USE_QT
    "Enable Qt platform integration for system theme detection"
    OFF
)

if(PINGDART_USE_QT)
    find_package(Qt6 6.5 QUIET COMPONENTS Core Gui)

    if(Qt6_FOUND)
        target_compile_definitions(pingdart_ui PUBLIC PINGDART_USE_QT)
        target_link_libraries(pingdart_ui PUBLIC Qt6::Core Qt6::Gui)
        message(STATUS "PingDart: Qt ${Qt6_VERSION} integration enabled (PINGDART_USE_QT)")
    else()
        message(WARNING
            "PingDart: PINGDART_USE_QT=ON but Qt 6.5+ was not found. "
            "SystemTheme Qt adapter will be compiled out."
        )
    endif()
endif()

add_library(PingDart::UI ALIAS pingdart_ui)

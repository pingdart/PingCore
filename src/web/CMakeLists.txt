# src/web/CMakeLists.txt

add_library(pingdart_web STATIC
    html/html_serializer.cpp
    html/semantic_widget.cpp
    ssr/default_web_renderer.cpp
    ssr/ssr_server.cpp
    ssg/route_enumerator.cpp
    ssg/static_generator.cpp
    dom/mock_dom_adapter.cpp
    dom/dom_reconciler.cpp
    dom/dom_event_dispatcher.cpp
    browser/browser_scheduler.cpp
    wasm/wasm_runtime.cpp
)

target_include_directories(pingdart_web
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pingdart_web
    PUBLIC
        PingDart::Core
)

add_library(PingDart::Web ALIAS pingdart_web)

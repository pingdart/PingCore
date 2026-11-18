// include/pingdart/ui.h
// PingDartUI — Unified Declarative UI & Styling Framework (PKG-135)
// Simple modern C++20 API: zero raw pointers, multi-styling, pluggable Qt/Native/Web renderers
#pragma once

#include <pingdart/ui/style.h>
#include <pingdart/ui/state.h>
#include <pingdart/ui/router.h>
#include <pingdart/ui/component.h>
#include <pingdart/ui/components.h>
#include <pingdart/ui/renderer.h>

namespace pingdart::ui {

// Core framework initialization & entry points
inline std::string render_to_html(const Component& root) {
    WebSsrAdapter adapter;
    return adapter.render_to_string(root);
}

inline std::string render_to_qml(const Component& root) {
    QtQuickAdapter adapter;
    return adapter.render_to_string(root);
}

} // namespace pingdart::ui

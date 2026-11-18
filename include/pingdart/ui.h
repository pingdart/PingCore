// include/pingdart/ui.h
// PingDartUI — Unified Declarative UI, Reactive Runtime & Styling Framework (PKG-138)
// Simple modern C++20 API: zero raw pointers, multi-styling, pluggable Qt/Native/Web renderers
#pragma once

#include <pingdart/ui/theme.h>
#include <pingdart/ui/css_parser.h>
#include <pingdart/ui/style.h>
#include <pingdart/ui/state.h>
#include <pingdart/ui/router.h>
#include <pingdart/ui/component.h>
#include <pingdart/ui/components.h>
#include <pingdart/ui/renderer.h>

// PKG-138 Events & Gestures
#include <pingdart/ui/events/event.h>
#include <pingdart/ui/events/pointer_event.h>
#include <pingdart/ui/events/keyboard_event.h>
#include <pingdart/ui/events/focus_manager.h>
#include <pingdart/ui/events/dispatcher.h>
#include <pingdart/ui/gestures/gesture.h>
#include <pingdart/ui/gestures/tap.h>
#include <pingdart/ui/gestures/swipe.h>
#include <pingdart/ui/gestures/pinch.h>
#include <pingdart/ui/gestures/drag.h>

// PKG-138 Reactive Runtime & Reconciler
#include <pingdart/ui/runtime/component_instance.h>
#include <pingdart/ui/runtime/lifecycle.h>
#include <pingdart/ui/runtime/dependency_tracker.h>
#include <pingdart/ui/runtime/scheduler.h>
#include <pingdart/ui/runtime/reconciler.h>
#include <pingdart/ui/runtime/application.h>

// PKG-138 React-like Hooks
#include <pingdart/ui/hooks/use_state.h>
#include <pingdart/ui/hooks/use_effect.h>
#include <pingdart/ui/hooks/use_memo.h>
#include <pingdart/ui/hooks/use_ref.h>
#include <pingdart/ui/hooks/use_context.h>
#include <pingdart/ui/hooks/use_reducer.h>
#include <pingdart/ui/hooks/use_store.h>
#include <pingdart/ui/hooks/use_theme.h>

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

// src/pal/windows/win32_input_dispatcher.h
#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <pingdart/pal/input_events.h>

namespace pingdart::pal::windows {

class Win32InputHelper {
public:
    static ModifierKey query_modifiers() noexcept;
    static PointerButton query_mouse_button(UINT msg, WPARAM wparam) noexcept;
    static uint8_t query_buttons_down(WPARAM wparam) noexcept;
    static uint32_t normalize_virtual_key(WPARAM wparam, LPARAM lparam) noexcept;
    static uint64_t current_timestamp_us() noexcept;
};

} // namespace pingdart::pal::windows

// src/pal/windows/win32_input_dispatcher.cpp
#include "win32_input_dispatcher.h"
#include <chrono>

namespace pingdart::pal::windows {

ModifierKey Win32InputHelper::query_modifiers() noexcept {
    ModifierKey mods = ModifierKey::None;
    if (GetKeyState(VK_SHIFT) & 0x8000) mods = mods | ModifierKey::Shift;
    if (GetKeyState(VK_CONTROL) & 0x8000) mods = mods | ModifierKey::Control;
    if (GetKeyState(VK_MENU) & 0x8000) mods = mods | ModifierKey::Alt;
    if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000)) mods = mods | ModifierKey::Meta;
    if (GetKeyState(VK_CAPITAL) & 0x0001) mods = mods | ModifierKey::CapsLock;
    return mods;
}

PointerButton Win32InputHelper::query_mouse_button(UINT msg, WPARAM wparam) noexcept {
    (void)wparam;
    switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            return PointerButton::Left;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            return PointerButton::Right;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            return PointerButton::Middle;
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            return (HIWORD(wparam) == XBUTTON1) ? PointerButton::Back : PointerButton::Forward;
        default:
            return PointerButton::None;
    }
}

uint8_t Win32InputHelper::query_buttons_down(WPARAM wparam) noexcept {
    uint8_t mask = 0;
    if (wparam & MK_LBUTTON) mask |= static_cast<uint8_t>(PointerButton::Left);
    if (wparam & MK_RBUTTON) mask |= static_cast<uint8_t>(PointerButton::Right);
    if (wparam & MK_MBUTTON) mask |= static_cast<uint8_t>(PointerButton::Middle);
    if (wparam & MK_XBUTTON1) mask |= static_cast<uint8_t>(PointerButton::Back);
    if (wparam & MK_XBUTTON2) mask |= static_cast<uint8_t>(PointerButton::Forward);
    return mask;
}

uint32_t Win32InputHelper::normalize_virtual_key(WPARAM wparam, LPARAM lparam) noexcept {
    (void)lparam;
    // Map Windows VK_* keys directly to standard scan codes / virtual keys
    return static_cast<uint32_t>(wparam);
}

uint64_t Win32InputHelper::current_timestamp_us() noexcept {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

} // namespace pingdart::pal::windows

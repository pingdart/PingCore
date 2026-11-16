// src/pal/linux/x11/x11_input_dispatcher.cpp
#include "x11_input_dispatcher.h"
#include <chrono>

#if defined(__linux__)
#include <X11/X.h>
#else
// X11 constants for non-Linux compilation
constexpr unsigned int ShiftMask   = (1 << 0);
constexpr unsigned int LockMask    = (1 << 1);
constexpr unsigned int ControlMask = (1 << 2);
constexpr unsigned int Mod1Mask    = (1 << 3);
constexpr unsigned int Mod4Mask    = (1 << 6);
constexpr unsigned int Button1     = 1;
constexpr unsigned int Button2     = 2;
constexpr unsigned int Button3     = 3;
[[maybe_unused]] constexpr unsigned int Button4 = 4;
[[maybe_unused]] constexpr unsigned int Button5 = 5;
#endif

namespace pingdart::pal::linux_x11 {

PointerButton X11InputHelper::map_x11_button(unsigned int button) noexcept {
    switch (button) {
        case Button1: return PointerButton::Left;
        case Button2: return PointerButton::Middle;
        case Button3: return PointerButton::Right;
        default:      return PointerButton::None;
    }
}

ModifierKey X11InputHelper::map_x11_modifiers(unsigned int state) noexcept {
    ModifierKey mods = ModifierKey::None;
    if (state & ShiftMask)   mods = mods | ModifierKey::Shift;
    if (state & LockMask)    mods = mods | ModifierKey::CapsLock;
    if (state & ControlMask) mods = mods | ModifierKey::Control;
    if (state & Mod1Mask)    mods = mods | ModifierKey::Alt;
    if (state & Mod4Mask)    mods = mods | ModifierKey::Meta;
    return mods;
}

uint64_t X11InputHelper::current_timestamp_us() noexcept {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

} // namespace pingdart::pal::linux_x11

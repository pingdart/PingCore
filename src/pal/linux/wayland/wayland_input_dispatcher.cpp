// src/pal/linux/wayland/wayland_input_dispatcher.cpp
#include "wayland_input_dispatcher.h"
#include <chrono>

namespace pingdart::pal::linux_wayland {

namespace {

// Linux evdev button codes (linux/input-event-codes.h)
constexpr uint32_t BTN_LEFT    = 0x110;
constexpr uint32_t BTN_RIGHT   = 0x111;
constexpr uint32_t BTN_MIDDLE  = 0x112;
constexpr uint32_t BTN_SIDE    = 0x113;
constexpr uint32_t BTN_EXTRA   = 0x114;

} // namespace

PointerButton WaylandInputHelper::map_linux_button(uint32_t linux_button) noexcept {
    switch (linux_button) {
        case BTN_LEFT:   return PointerButton::Left;
        case BTN_RIGHT:  return PointerButton::Right;
        case BTN_MIDDLE: return PointerButton::Middle;
        case BTN_SIDE:   return PointerButton::Back;
        case BTN_EXTRA:  return PointerButton::Forward;
        default:         return PointerButton::None;
    }
}

ModifierKey WaylandInputHelper::map_xkb_modifiers(
    uint32_t depressed,
    uint32_t latched,
    uint32_t locked
) noexcept {
    uint32_t all = depressed | latched | locked;
    ModifierKey mods = ModifierKey::None;

    if (all & (1 << 0)) mods = mods | ModifierKey::Shift;
    if (all & (1 << 1)) mods = mods | ModifierKey::CapsLock;
    if (all & (1 << 2)) mods = mods | ModifierKey::Control;
    if (all & (1 << 3)) mods = mods | ModifierKey::Alt;
    if (all & (1 << 6)) mods = mods | ModifierKey::Meta;

    return mods;
}

uint64_t WaylandInputHelper::current_timestamp_us() noexcept {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

} // namespace pingdart::pal::linux_wayland

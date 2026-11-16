// src/pal/linux/wayland/wayland_input_dispatcher.h
#pragma once
#include <pingdart/pal/input_events.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <cstdint>

namespace pingdart::pal::linux_wayland {

class WaylandInputHelper {
public:
    static PointerButton map_linux_button(uint32_t linux_button) noexcept;
    static ModifierKey map_xkb_modifiers(uint32_t depressed, uint32_t latched, uint32_t locked) noexcept;
    static uint64_t current_timestamp_us() noexcept;
};

} // namespace pingdart::pal::linux_wayland

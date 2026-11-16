// src/pal/linux/x11/x11_input_dispatcher.h
#pragma once
#include <pingdart/pal/input_events.h>
#include <cstdint>

namespace pingdart::pal::linux_x11 {

class X11InputHelper {
public:
    static PointerButton map_x11_button(unsigned int button) noexcept;
    static ModifierKey map_x11_modifiers(unsigned int state) noexcept;
    static uint64_t current_timestamp_us() noexcept;
};

} // namespace pingdart::pal::linux_x11

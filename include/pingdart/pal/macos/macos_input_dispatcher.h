// include/pingdart/pal/macos/macos_input_dispatcher.h
// PKG-113: macOS Native Input Translator (NSEvent -> PingDart Events)
#pragma once
#include <pingdart/pal/input_events.h>
#include <pingdart/pal/i_input_dispatcher.h>
#include <cstdint>
#include <string_view>

namespace pingdart::pal::macos {

class MacosInputTranslator {
public:
    // Translates Cocoa NSEventModifierFlags to PingDart ModifierKey bitmask
    static ModifierKey translate_modifiers(uint64_t modifier_flags) noexcept;

    // Translates macOS virtual key code (kVK_ANSI_*) to PingDart logical key
    static uint32_t translate_virtual_key(uint16_t key_code) noexcept;

    // Translates Cocoa button numbers (0=left, 1=right, 2=middle, etc.) to PointerButton
    static PointerButton translate_button(int64_t button_number) noexcept;
};

} // namespace pingdart::pal::macos

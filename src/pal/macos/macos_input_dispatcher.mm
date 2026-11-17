// src/pal/macos/macos_input_dispatcher.mm
// PKG-113: macOS Native Input Translator Implementation
#include <pingdart/pal/macos/macos_input_dispatcher.h>
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

namespace pingdart::pal::macos {

ModifierKey MacosInputTranslator::translate_modifiers(uint64_t modifier_flags) noexcept {
    ModifierKey mods = ModifierKey::None;
    if (modifier_flags & NSEventModifierFlagShift) {
        mods = mods | ModifierKey::Shift;
    }
    if (modifier_flags & NSEventModifierFlagControl) {
        mods = mods | ModifierKey::Control;
    }
    if (modifier_flags & NSEventModifierFlagOption) {
        mods = mods | ModifierKey::Alt;
    }
    if (modifier_flags & NSEventModifierFlagCommand) {
        mods = mods | ModifierKey::Meta;
    }
    if (modifier_flags & NSEventModifierFlagCapsLock) {
        mods = mods | ModifierKey::CapsLock;
    }
    return mods;
}

PointerButton MacosInputTranslator::translate_button(int64_t button_number) noexcept {
    switch (button_number) {
        case 0: return PointerButton::Left;
        case 1: return PointerButton::Right;
        case 2: return PointerButton::Middle;
        case 3: return PointerButton::Back;
        case 4: return PointerButton::Forward;
        default: return PointerButton::None;
    }
}

uint32_t MacosInputTranslator::translate_virtual_key(uint16_t key_code) noexcept {
    switch (key_code) {
        case kVK_ANSI_A: return 'A';
        case kVK_ANSI_B: return 'B';
        case kVK_ANSI_C: return 'C';
        case kVK_ANSI_D: return 'D';
        case kVK_ANSI_E: return 'E';
        case kVK_ANSI_F: return 'F';
        case kVK_ANSI_G: return 'G';
        case kVK_ANSI_H: return 'H';
        case kVK_ANSI_I: return 'I';
        case kVK_ANSI_J: return 'J';
        case kVK_ANSI_K: return 'K';
        case kVK_ANSI_L: return 'L';
        case kVK_ANSI_M: return 'M';
        case kVK_ANSI_N: return 'N';
        case kVK_ANSI_O: return 'O';
        case kVK_ANSI_P: return 'P';
        case kVK_ANSI_Q: return 'Q';
        case kVK_ANSI_R: return 'R';
        case kVK_ANSI_S: return 'S';
        case kVK_ANSI_T: return 'T';
        case kVK_ANSI_U: return 'U';
        case kVK_ANSI_V: return 'V';
        case kVK_ANSI_W: return 'W';
        case kVK_ANSI_X: return 'X';
        case kVK_ANSI_Y: return 'Y';
        case kVK_ANSI_Z: return 'Z';

        case kVK_ANSI_0: return '0';
        case kVK_ANSI_1: return '1';
        case kVK_ANSI_2: return '2';
        case kVK_ANSI_3: return '3';
        case kVK_ANSI_4: return '4';
        case kVK_ANSI_5: return '5';
        case kVK_ANSI_6: return '6';
        case kVK_ANSI_7: return '7';
        case kVK_ANSI_8: return '8';
        case kVK_ANSI_9: return '9';

        case kVK_Return: return 13;      // Enter / Return
        case kVK_Tab: return 9;          // Tab
        case kVK_Space: return 32;       // Space
        case kVK_Delete: return 8;       // Backspace
        case kVK_Escape: return 27;      // Escape

        case kVK_LeftArrow:  return 37;  // Left
        case kVK_UpArrow:    return 38;  // Up
        case kVK_RightArrow: return 39;  // Right
        case kVK_DownArrow:  return 40;  // Down

        case kVK_Home: return 36;
        case kVK_End: return 35;
        case kVK_PageUp: return 33;
        case kVK_PageDown: return 34;

        case kVK_F1:  return 112;
        case kVK_F2:  return 113;
        case kVK_F3:  return 114;
        case kVK_F4:  return 115;
        case kVK_F5:  return 116;
        case kVK_F6:  return 117;
        case kVK_F7:  return 118;
        case kVK_F8:  return 119;
        case kVK_F9:  return 120;
        case kVK_F10: return 121;
        case kVK_F11: return 122;
        case kVK_F12: return 123;

        case kVK_Shift:
        case kVK_RightShift: return 16;
        case kVK_Control:
        case kVK_RightControl: return 17;
        case kVK_Option:
        case kVK_RightOption: return 18;
        case kVK_Command: return 91;

        default: return key_code;
    }
}

} // namespace pingdart::pal::macos

// include/pingdart/pal/input_events.h
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace pingdart::pal {

enum class EventType : uint16_t {
    PointerDown,
    PointerUp,
    PointerMove,
    PointerEnter,
    PointerLeave,
    PointerWheel,
    KeyDown,
    KeyUp,
    CharacterInput,
    WindowFocusGained,
    WindowFocusLost,
    WindowResized,
    WindowMoved,
    WindowCloseRequested,
    WindowDpiChanged
};

enum class KeyAction : uint8_t {
    Down,
    Up,
    Repeat
};

enum class TextEventType : uint8_t {
    CompositionStart,
    CompositionUpdate,
    CompositionCommit,
    CompositionEnd
};

enum class CursorKind : uint8_t {
    Arrow,
    IBeam,
    PointingHand,
    Crosshair,
    ResizeHorizontal,
    ResizeVertical,
    Wait,
    Hidden
};

enum class PointerButton : uint8_t {
    None = 0,
    Left = 1,
    Right = 2,
    Middle = 3,
    Back = 4,
    Forward = 5
};

enum class ModifierKey : uint8_t {
    None     = 0,
    Shift    = 1 << 0,
    Control  = 1 << 1,
    Alt      = 1 << 2,
    Meta     = 1 << 3, // Windows Key / Command / Super
    CapsLock = 1 << 4
};

inline ModifierKey operator|(ModifierKey a, ModifierKey b) noexcept {
    return static_cast<ModifierKey>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool operator&(ModifierKey a, ModifierKey b) noexcept {
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

struct PointerEvent {
    EventType type{EventType::PointerMove};
    uint32_t pointer_id{0};       // 0 for mouse, 1+ for touch/stylus
    double x{0.0};                // Window relative X coordinate
    double y{0.0};                // Window relative Y coordinate
    double delta_x{0.0};          // Mouse wheel horizontal scroll delta
    double delta_y{0.0};          // Mouse wheel vertical scroll delta
    PointerButton button{PointerButton::None};
    uint8_t buttons_down{0};      // Bitmask of active PointerButton values
    ModifierKey modifiers{ModifierKey::None};
    float pressure{1.0f};         // 0.0f to 1.0f
    uint64_t timestamp_us{0};     // Microseconds since steady_clock epoch
};

struct KeyboardEvent {
    EventType type{EventType::KeyDown};
    uint32_t key_code{0};         // Hardware scancode
    uint32_t virtual_key{0};      // Normalized virtual key
    uint32_t unicode_codepoint{0};// UTF-32 character for CharacterInput
    ModifierKey modifiers{ModifierKey::None};
    bool is_repeat{false};
    uint64_t timestamp_us{0};
};

struct KeyEvent {
    uint32_t physical_key{0};    // Hardware scan code / evdev code
    uint32_t logical_key{0};     // Normalized virtual key / keysym
    KeyAction action{KeyAction::Down};
    uint32_t modifiers{0};       // Bitmask of ModifierKey
    uint64_t timestamp_ns{0};    // Nanoseconds since steady_clock epoch
};

struct TextInputEvent {
    TextEventType type{TextEventType::CompositionCommit};
    std::string utf8_text;
    uint32_t selection_start{0}; // UTF-8 byte position
    uint32_t selection_end{0};   // UTF-8 byte position
};

enum class TouchPhase : uint8_t {
    Began,
    Moved,
    Stationary,
    Ended,
    Cancelled
};

struct TouchPoint {
    uint32_t touch_id{0};
    double x{0.0};
    double y{0.0};
    float pressure{1.0f};
    TouchPhase phase{TouchPhase::Began};
    uint64_t timestamp_us{0};
};

struct TouchEvent {
    EventType type{EventType::PointerDown};
    TouchPhase phase{TouchPhase::Began};
    TouchPoint primary_touch{};
    std::vector<TouchPoint> all_touches{};
    uint64_t timestamp_us{0};
};

struct WindowLifecycleEvent {
    EventType type{EventType::WindowResized};
    uint32_t width{0};
    uint32_t height{0};
    float scale_factor{1.0f};
    uint64_t timestamp_us{0};
};

class ITextInputClient {
public:
    virtual ~ITextInputClient() = default;

    virtual void on_text_input(const TextInputEvent& event) = 0;
    virtual void on_focus_changed(bool focused) = 0;
};

} // namespace pingdart::pal

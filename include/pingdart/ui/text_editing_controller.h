#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>
#include <cstdint>

namespace pingdart::ui {

struct TextSelection {
    int32_t base_offset{0};
    int32_t extent_offset{0};

    [[nodiscard]] constexpr bool is_collapsed() const noexcept {
        return base_offset == extent_offset;
    }

    [[nodiscard]] constexpr int32_t start() const noexcept {
        return std::min(base_offset, extent_offset);
    }

    [[nodiscard]] constexpr int32_t end() const noexcept {
        return std::max(base_offset, extent_offset);
    }

    [[nodiscard]] static constexpr TextSelection collapsed(int32_t offset) noexcept {
        return TextSelection{offset, offset};
    }

    constexpr bool operator==(const TextSelection& o) const noexcept {
        return base_offset == o.base_offset && extent_offset == o.extent_offset;
    }
};

struct TextRange {
    int32_t start{-1};
    int32_t end{-1};

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return start >= 0 && end >= start;
    }

    [[nodiscard]] static constexpr TextRange empty() noexcept {
        return TextRange{-1, -1};
    }

    constexpr bool operator==(const TextRange& o) const noexcept {
        return start == o.start && end == o.end;
    }
};

struct TextEditingValue {
    std::string text;
    TextSelection selection{0, 0};
    TextRange composing_range{-1, -1};

    bool operator==(const TextEditingValue& o) const noexcept {
        return text == o.text && selection == o.selection && composing_range == o.composing_range;
    }
};

class UnicodeGraphemeUtils {
public:
    // Decodes next UTF-32 codepoint from UTF-8 string view at byte index 'offset'
    static uint32_t decode_next_codepoint(std::string_view text, size_t& offset);

    // Decodes previous UTF-32 codepoint before byte index 'offset'
    static uint32_t decode_prev_codepoint(std::string_view text, size_t& offset);

    // Checks whether codepoint is a Unicode combining mark, virama, or vowel sign
    static bool is_combining_mark(uint32_t cp) noexcept;

    // Checks whether codepoint is an Indic Virama / Halant (explicit conjunct creator)
    static bool is_virama(uint32_t cp) noexcept;

    // Checks whether codepoint is a Regional Indicator Symbol (for flags)
    static bool is_regional_indicator(uint32_t cp) noexcept;

    // Checks whether codepoint is an Extended Pictographic symbol (for emoji sequences)
    static bool is_extended_pictographic(uint32_t cp) noexcept;

    // Finds previous user-perceived grapheme cluster boundary (safe for Backspace)
    static size_t previous_grapheme_boundary(std::string_view text, size_t byte_offset);

    // Finds next user-perceived grapheme cluster boundary (safe for Delete / Cursor Right)
    static size_t next_grapheme_boundary(std::string_view text, size_t byte_offset);

    // Counts total user-perceived grapheme clusters in UTF-8 text
    static size_t count_graphemes(std::string_view text);
};

class TextEditingController : public std::enable_shared_from_this<TextEditingController> {
public:
    explicit TextEditingController(std::string initial_text = "");
    virtual ~TextEditingController() = default;

    TextEditingController(const TextEditingController&) = delete;
    TextEditingController& operator=(const TextEditingController&) = delete;

    [[nodiscard]] const std::string& text() const noexcept { return value_.text; }
    void set_text(std::string text);

    [[nodiscard]] const TextSelection& selection() const noexcept { return value_.selection; }
    void set_selection(TextSelection selection);

    [[nodiscard]] const TextEditingValue& value() const noexcept { return value_; }
    void set_value(TextEditingValue value);

    // Inserts text at current caret or replaces active selection
    void insert_text(std::string_view str);

    // Deletes backward: selection if active, otherwise one full grapheme cluster
    void delete_backward();

    // Deletes forward: selection if active, otherwise one full grapheme cluster
    void delete_forward();

    // Caret movement
    void move_cursor_left(bool extend_selection = false);
    void move_cursor_right(bool extend_selection = false);
    void move_cursor_to_start(bool extend_selection = false);
    void move_cursor_to_end(bool extend_selection = false);
    void select_all();

    // IME Composition Support
    void set_composition(std::string_view comp_text, int32_t sel_start = -1, int32_t sel_end = -1);
    void commit_composition(std::string_view final_text);
    void cancel_composition();

    void clear();

    void add_listener(std::function<void(const TextEditingValue&)> listener);

private:
    void notify_listeners();

    TextEditingValue value_;
    std::vector<std::function<void(const TextEditingValue&)>> listeners_;
};

} // namespace pingdart::ui

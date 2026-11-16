#include <pingdart/ui/text_editing_controller.h>
#include <cassert>

namespace pingdart::ui {

// -------------------------------------------------------------------------
// Unicode Grapheme Utilities
// -------------------------------------------------------------------------

uint32_t UnicodeGraphemeUtils::decode_next_codepoint(std::string_view text, size_t& offset) {
    if (offset >= text.size()) return 0;

    auto b0 = static_cast<uint8_t>(text[offset]);
    if (b0 < 0x80) {
        offset += 1;
        return b0;
    }
    if ((b0 & 0xE0) == 0xC0) {
        if (offset + 1 >= text.size()) { offset = text.size(); return 0xFFFD; }
        auto b1 = static_cast<uint8_t>(text[offset + 1]);
        offset += 2;
        return ((static_cast<uint32_t>(b0 & 0x1F) << 6) |
                static_cast<uint32_t>(b1 & 0x3F));
    }
    if ((b0 & 0xF0) == 0xE0) {
        if (offset + 2 >= text.size()) { offset = text.size(); return 0xFFFD; }
        auto b1 = static_cast<uint8_t>(text[offset + 1]);
        auto b2 = static_cast<uint8_t>(text[offset + 2]);
        offset += 3;
        return ((static_cast<uint32_t>(b0 & 0x0F) << 12) |
                (static_cast<uint32_t>(b1 & 0x3F) << 6) |
                static_cast<uint32_t>(b2 & 0x3F));
    }
    if ((b0 & 0xF8) == 0xF0) {
        if (offset + 3 >= text.size()) { offset = text.size(); return 0xFFFD; }
        auto b1 = static_cast<uint8_t>(text[offset + 1]);
        auto b2 = static_cast<uint8_t>(text[offset + 2]);
        auto b3 = static_cast<uint8_t>(text[offset + 3]);
        offset += 4;
        return ((static_cast<uint32_t>(b0 & 0x07) << 18) |
                (static_cast<uint32_t>(b1 & 0x3F) << 12) |
                (static_cast<uint32_t>(b2 & 0x3F) << 6) |
                static_cast<uint32_t>(b3 & 0x3F));
    }

    offset += 1;
    return 0xFFFD;
}

uint32_t UnicodeGraphemeUtils::decode_prev_codepoint(std::string_view text, size_t& offset) {
    if (offset == 0) return 0;

    size_t pos = offset - 1;
    // Walk back over continuation bytes (10xxxxxx)
    while (pos > 0 && (static_cast<uint8_t>(text[pos]) & 0xC0) == 0x80) {
        pos--;
    }
    offset = pos;
    size_t tmp = pos;
    return decode_next_codepoint(text, tmp);
}

bool UnicodeGraphemeUtils::is_virama(uint32_t cp) noexcept {
    return (cp == 0x094D || // Devanagari Virama
            cp == 0x0C4D || // Telugu Virama
            cp == 0x0BCD || // Tamil Virama
            cp == 0x0A4D || // Gurmukhi Virama
            cp == 0x0ACD || // Gujarati Virama
            cp == 0x0B4D || // Oriya Virama
            cp == 0x0CCD || // Kannada Virama
            cp == 0x0D4D);  // Malayalam Virama
}

bool UnicodeGraphemeUtils::is_regional_indicator(uint32_t cp) noexcept {
    return (cp >= 0x1F1E6 && cp <= 0x1F1FF);
}

bool UnicodeGraphemeUtils::is_extended_pictographic(uint32_t cp) noexcept {
    if (cp >= 0x1F300 && cp <= 0x1FAFF) return true;
    if (cp >= 0x2600 && cp <= 0x27BF) return true;
    if (cp >= 0x2B50 && cp <= 0x2B55) return true;
    if (cp == 0x00A9 || cp == 0x00AE) return true;
    return false;
}

bool UnicodeGraphemeUtils::is_combining_mark(uint32_t cp) noexcept {
    // Combining Diacritical Marks
    if (cp >= 0x0300 && cp <= 0x036F) return true;
    // Devanagari combining signs, matras, virama
    if (cp >= 0x0900 && cp <= 0x0903) return true;
    if (cp >= 0x093A && cp <= 0x094F) return true;
    if (cp >= 0x0951 && cp <= 0x0957) return true;
    if (cp >= 0x0962 && cp <= 0x0963) return true;
    // Telugu combining signs, matras, virama, length marks
    if (cp >= 0x0C00 && cp <= 0x0C03) return true;
    if (cp >= 0x0C3E && cp <= 0x0C4D) return true;
    if (cp >= 0x0C55 && cp <= 0x0C56) return true;
    if (cp >= 0x0C62 && cp <= 0x0C63) return true;
    // Arabic Tashkeel
    if (cp >= 0x064B && cp <= 0x065F) return true;
    if (cp == 0x0670) return true;
    // ZWJ, ZWNJ
    if (cp == 0x200C || cp == 0x200D) return true;
    // Variation selectors
    if (cp >= 0xFE00 && cp <= 0xFE0F) return true;
    if (cp >= 0xE0100 && cp <= 0xE01EF) return true;
    // Emoji skin tones
    if (cp >= 0x1F3FB && cp <= 0x1F3FF) return true;

    return false;
}

size_t UnicodeGraphemeUtils::previous_grapheme_boundary(std::string_view text, size_t byte_offset) {
    if (byte_offset == 0) return 0;
    if (byte_offset > text.size()) byte_offset = text.size();

    size_t curr = byte_offset;
    uint32_t cp = decode_prev_codepoint(text, curr);

    // CR x LF
    if (cp == '\n' && curr > 0 && text[curr - 1] == '\r') {
        return curr - 1;
    }

    // Walk back over combining marks, ZWJ, viramas, matras
    while (curr > 0) {
        if (is_combining_mark(cp) || cp == 0x200D) {
            cp = decode_prev_codepoint(text, curr);
        } else {
            break;
        }
    }

    // If preceded by a virama, part of an Indic conjunct
    while (curr > 0) {
        size_t lookback = curr;
        uint32_t prev_cp = decode_prev_codepoint(text, lookback);
        if (is_virama(prev_cp)) {
            curr = lookback;
            if (curr > 0) {
                decode_prev_codepoint(text, curr);
            }
        } else if (prev_cp == 0x200D && is_extended_pictographic(cp)) {
            curr = lookback;
            if (curr > 0) {
                decode_prev_codepoint(text, curr);
            }
        } else {
            break;
        }
    }

    // Regional Indicator pair (flags like 🇮🇳)
    if (is_regional_indicator(cp) && curr > 0) {
        size_t lookback = curr;
        uint32_t prev_cp = decode_prev_codepoint(text, lookback);
        if (is_regional_indicator(prev_cp)) {
            curr = lookback;
        }
    }

    return curr;
}

size_t UnicodeGraphemeUtils::next_grapheme_boundary(std::string_view text, size_t byte_offset) {
    if (byte_offset >= text.size()) return text.size();

    size_t curr = byte_offset;
    uint32_t base_cp = decode_next_codepoint(text, curr);

    // Rule: CR x LF
    if (base_cp == '\r' && curr < text.size() && text[curr] == '\n') {
        return curr + 1;
    }
    if (base_cp == '\r' || base_cp == '\n') {
        return curr;
    }

    // Rule: Regional Indicators (pair of 2)
    if (is_regional_indicator(base_cp)) {
        size_t lookahead = curr;
        uint32_t next_cp = decode_next_codepoint(text, lookahead);
        if (is_regional_indicator(next_cp)) {
            return lookahead;
        }
        return curr;
    }

    // Rule: Combining marks, matras, viramas & Emoji ZWJ sequences
    while (curr < text.size()) {
        size_t lookahead = curr;
        uint32_t next_cp = decode_next_codepoint(text, lookahead);

        // Emoji ZWJ sequence (ZWJ + Pictographic)
        if (next_cp == 0x200D && lookahead < text.size()) {
            size_t after_zwj = lookahead;
            uint32_t emoji_cp = decode_next_codepoint(text, after_zwj);
            if (is_extended_pictographic(emoji_cp)) {
                curr = after_zwj;
                continue;
            }
        }

        if (is_combining_mark(next_cp)) {
            curr = lookahead;
            if (is_virama(next_cp) && curr < text.size()) {
                decode_next_codepoint(text, curr);
            }
            continue;
        }

        break;
    }

    return curr;
}

size_t UnicodeGraphemeUtils::count_graphemes(std::string_view text) {
    size_t count = 0;
    size_t offset = 0;
    while (offset < text.size()) {
        offset = next_grapheme_boundary(text, offset);
        count++;
    }
    return count;
}

// -------------------------------------------------------------------------
// TextEditingController
// -------------------------------------------------------------------------

TextEditingController::TextEditingController(std::string initial_text) {
    int32_t len = static_cast<int32_t>(initial_text.size());
    value_.text = std::move(initial_text);
    value_.selection = TextSelection::collapsed(len);
    value_.composing_range = TextRange::empty();
}

void TextEditingController::set_text(std::string text) {
    value_.text = std::move(text);
    int32_t len = static_cast<int32_t>(value_.text.size());
    value_.selection = TextSelection::collapsed(len);
    value_.composing_range = TextRange::empty();
    notify_listeners();
}

void TextEditingController::set_selection(TextSelection selection) {
    int32_t len = static_cast<int32_t>(value_.text.size());
    value_.selection.base_offset = std::clamp(selection.base_offset, 0, len);
    value_.selection.extent_offset = std::clamp(selection.extent_offset, 0, len);
    notify_listeners();
}

void TextEditingController::set_value(TextEditingValue value) {
    int32_t len = static_cast<int32_t>(value.text.size());
    value.selection.base_offset = std::clamp(value.selection.base_offset, 0, len);
    value.selection.extent_offset = std::clamp(value.selection.extent_offset, 0, len);
    value_ = std::move(value);
    notify_listeners();
}

void TextEditingController::insert_text(std::string_view str) {
    int32_t start = value_.selection.start();
    int32_t end = value_.selection.end();

    value_.text.replace(static_cast<size_t>(start), static_cast<size_t>(end - start), str);
    int32_t new_cursor = start + static_cast<int32_t>(str.size());
    value_.selection = TextSelection::collapsed(new_cursor);
    value_.composing_range = TextRange::empty();
    notify_listeners();
}

void TextEditingController::delete_backward() {
    if (!value_.selection.is_collapsed()) {
        insert_text("");
        return;
    }

    int32_t cursor = value_.selection.base_offset;
    if (cursor <= 0) return;

    size_t prev_boundary = UnicodeGraphemeUtils::previous_grapheme_boundary(
        value_.text, static_cast<size_t>(cursor));

    size_t count = static_cast<size_t>(cursor) - prev_boundary;
    value_.text.erase(prev_boundary, count);
    value_.selection = TextSelection::collapsed(static_cast<int32_t>(prev_boundary));
    value_.composing_range = TextRange::empty();
    notify_listeners();
}

void TextEditingController::delete_forward() {
    if (!value_.selection.is_collapsed()) {
        insert_text("");
        return;
    }

    int32_t cursor = value_.selection.base_offset;
    if (cursor >= static_cast<int32_t>(value_.text.size())) return;

    size_t next_boundary = UnicodeGraphemeUtils::next_grapheme_boundary(
        value_.text, static_cast<size_t>(cursor));

    size_t count = next_boundary - static_cast<size_t>(cursor);
    value_.text.erase(static_cast<size_t>(cursor), count);
    value_.selection = TextSelection::collapsed(cursor);
    value_.composing_range = TextRange::empty();
    notify_listeners();
}

void TextEditingController::move_cursor_left(bool extend_selection) {
    int32_t cursor = value_.selection.extent_offset;
    if (!extend_selection && !value_.selection.is_collapsed()) {
        value_.selection = TextSelection::collapsed(value_.selection.start());
        notify_listeners();
        return;
    }

    if (cursor > 0) {
        size_t prev_boundary = UnicodeGraphemeUtils::previous_grapheme_boundary(
            value_.text, static_cast<size_t>(cursor));
        int32_t new_offset = static_cast<int32_t>(prev_boundary);
        if (extend_selection) {
            value_.selection.extent_offset = new_offset;
        } else {
            value_.selection = TextSelection::collapsed(new_offset);
        }
        notify_listeners();
    }
}

void TextEditingController::move_cursor_right(bool extend_selection) {
    int32_t cursor = value_.selection.extent_offset;
    if (!extend_selection && !value_.selection.is_collapsed()) {
        value_.selection = TextSelection::collapsed(value_.selection.end());
        notify_listeners();
        return;
    }

    if (cursor < static_cast<int32_t>(value_.text.size())) {
        size_t next_boundary = UnicodeGraphemeUtils::next_grapheme_boundary(
            value_.text, static_cast<size_t>(cursor));
        int32_t new_offset = static_cast<int32_t>(next_boundary);
        if (extend_selection) {
            value_.selection.extent_offset = new_offset;
        } else {
            value_.selection = TextSelection::collapsed(new_offset);
        }
        notify_listeners();
    }
}

void TextEditingController::move_cursor_to_start(bool extend_selection) {
    if (extend_selection) {
        value_.selection.extent_offset = 0;
    } else {
        value_.selection = TextSelection::collapsed(0);
    }
    notify_listeners();
}

void TextEditingController::move_cursor_to_end(bool extend_selection) {
    int32_t end = static_cast<int32_t>(value_.text.size());
    if (extend_selection) {
        value_.selection.extent_offset = end;
    } else {
        value_.selection = TextSelection::collapsed(end);
    }
    notify_listeners();
}

void TextEditingController::select_all() {
    value_.selection = TextSelection{0, static_cast<int32_t>(value_.text.size())};
    notify_listeners();
}

void TextEditingController::set_composition(std::string_view comp_text, int32_t sel_start, int32_t sel_end) {
    int32_t start = value_.composing_range.is_valid() ? value_.composing_range.start : value_.selection.start();
    int32_t end = value_.composing_range.is_valid() ? value_.composing_range.end : value_.selection.end();

    value_.text.replace(static_cast<size_t>(start), static_cast<size_t>(end - start), comp_text);
    int32_t new_comp_end = start + static_cast<int32_t>(comp_text.size());
    value_.composing_range = TextRange{start, new_comp_end};

    if (sel_start >= 0 && sel_end >= 0) {
        value_.selection = TextSelection{start + sel_start, start + sel_end};
    } else {
        value_.selection = TextSelection::collapsed(new_comp_end);
    }
    notify_listeners();
}

void TextEditingController::commit_composition(std::string_view final_text) {
    set_composition(final_text);
    value_.composing_range = TextRange::empty();
    notify_listeners();
}

void TextEditingController::cancel_composition() {
    if (value_.composing_range.is_valid()) {
        value_.text.erase(static_cast<size_t>(value_.composing_range.start),
                          static_cast<size_t>(value_.composing_range.end - value_.composing_range.start));
        value_.selection = TextSelection::collapsed(value_.composing_range.start);
        value_.composing_range = TextRange::empty();
        notify_listeners();
    }
}

void TextEditingController::clear() {
    value_.text.clear();
    value_.selection = TextSelection::collapsed(0);
    value_.composing_range = TextRange::empty();
    notify_listeners();
}

void TextEditingController::add_listener(std::function<void(const TextEditingValue&)> listener) {
    listeners_.push_back(std::move(listener));
}

void TextEditingController::notify_listeners() {
    for (const auto& listener : listeners_) {
        if (listener) {
            listener(value_);
        }
    }
}

} // namespace pingdart::ui

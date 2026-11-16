#include <pingdart/text/unicode_analyzer.h>

namespace pingdart::text {

uint32_t UnicodeAnalyzer::decode_next_codepoint(std::string_view utf8, size_t& byte_offset) noexcept {
    if (byte_offset >= utf8.size()) return 0;

    auto b0 = static_cast<uint8_t>(utf8[byte_offset]);
    if (b0 < 0x80) {
        byte_offset += 1;
        return b0;
    } else if ((b0 & 0xE0) == 0xC0) {
        if (byte_offset + 1 >= utf8.size()) { byte_offset = utf8.size(); return 0xFFFD; }
        auto b1 = static_cast<uint8_t>(utf8[byte_offset + 1]);
        byte_offset += 2;
        return static_cast<uint32_t>((b0 & 0x1F) << 6) | static_cast<uint32_t>(b1 & 0x3F);
    } else if ((b0 & 0xF0) == 0xE0) {
        if (byte_offset + 2 >= utf8.size()) { byte_offset = utf8.size(); return 0xFFFD; }
        auto b1 = static_cast<uint8_t>(utf8[byte_offset + 1]);
        auto b2 = static_cast<uint8_t>(utf8[byte_offset + 2]);
        byte_offset += 3;
        return static_cast<uint32_t>((b0 & 0x0F) << 12) |
               static_cast<uint32_t>((b1 & 0x3F) << 6) |
               static_cast<uint32_t>(b2 & 0x3F);
    } else if ((b0 & 0xF8) == 0xF0) {
        if (byte_offset + 3 >= utf8.size()) { byte_offset = utf8.size(); return 0xFFFD; }
        auto b1 = static_cast<uint8_t>(utf8[byte_offset + 1]);
        auto b2 = static_cast<uint8_t>(utf8[byte_offset + 2]);
        auto b3 = static_cast<uint8_t>(utf8[byte_offset + 3]);
        byte_offset += 4;
        return static_cast<uint32_t>((b0 & 0x07) << 18) |
               static_cast<uint32_t>((b1 & 0x3F) << 12) |
               static_cast<uint32_t>((b2 & 0x3F) << 6) |
               static_cast<uint32_t>(b3 & 0x3F);
    }

    byte_offset += 1;
    return 0xFFFD;
}

TextScript UnicodeAnalyzer::detect_script(uint32_t cp) noexcept {
    if (cp >= 0x0C00 && cp <= 0x0C7F) return TextScript::Telugu;
    if (cp >= 0x0900 && cp <= 0x097F) return TextScript::Devanagari;
    if ((cp >= 0x0600 && cp <= 0x06FF) || (cp >= 0x0750 && cp <= 0x077F) ||
        (cp >= 0x08A0 && cp <= 0x08FF) || (cp >= 0xFB50 && cp <= 0xFDFF) ||
        (cp >= 0xFE70 && cp <= 0xFEFF)) {
        return TextScript::Arabic;
    }
    if ((cp >= 0x1F300 && cp <= 0x1FAFF) || (cp >= 0x2600 && cp <= 0x27BF)) {
        return TextScript::Emoji;
    }
    if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z') ||
        (cp >= 0x00C0 && cp <= 0x024F) || (cp >= 0x1E00 && cp <= 0x1EFF)) {
        return TextScript::Latin;
    }
    return TextScript::Common;
}

TextDirection UnicodeAnalyzer::detect_direction(uint32_t cp) noexcept {
    return (detect_script(cp) == TextScript::Arabic) ? TextDirection::RTL : TextDirection::LTR;
}

TextDirection UnicodeAnalyzer::script_direction(TextScript script) noexcept {
    return (script == TextScript::Arabic) ? TextDirection::RTL : TextDirection::LTR;
}

bool UnicodeAnalyzer::is_combining_mark(uint32_t cp) noexcept {
    // Telugu vowel signs, virama, anusvara
    if ((cp >= 0x0C3E && cp <= 0x0C4D) || cp == 0x0C02 || cp == 0x0C03 ||
        cp == 0x0C55 || cp == 0x0C56) {
        return true;
    }
    // Devanagari vowel signs, virama, anusvara
    if ((cp >= 0x093E && cp <= 0x094D) || cp == 0x0902 || cp == 0x0903) {
        return true;
    }
    // Arabic tashkeel / harakat
    if ((cp >= 0x064B && cp <= 0x065F) || cp == 0x0670) {
        return true;
    }
    // Combining Diacritical Marks
    if (cp >= 0x0300 && cp <= 0x036F) {
        return true;
    }
    // ZWJ, ZWNJ
    if (cp == 0x200C || cp == 0x200D) {
        return true;
    }
    return false;
}

std::vector<GraphemeCluster> UnicodeAnalyzer::segment_grapheme_clusters(std::string_view utf8) {
    std::vector<GraphemeCluster> clusters;
    if (utf8.empty()) return clusters;

    size_t offset = 0;
    while (offset < utf8.size()) {
        size_t cluster_start = offset;
        (void)decode_next_codepoint(utf8, offset);
        uint32_t count = 1;
        bool has_combining = false;

        // Peak ahead and consume all trailing combining marks / virama conjuncts
        while (offset < utf8.size()) {
            size_t peak_offset = offset;
            uint32_t next_cp = decode_next_codepoint(utf8, peak_offset);

            if (is_combining_mark(next_cp)) {
                has_combining = true;
                count++;
                offset = peak_offset;
                // If this combining mark is a virama (halant), also consume the next consonant!
                if ((next_cp == 0x0C4D || next_cp == 0x094D) && offset < utf8.size()) {
                    decode_next_codepoint(utf8, offset);
                    count++;
                }
            } else {
                break;
            }
        }

        clusters.push_back(GraphemeCluster{
            .byte_start = cluster_start,
            .byte_length = offset - cluster_start,
            .codepoint_count = count,
            .is_combining_sequence = has_combining
        });
    }

    return clusters;
}

std::vector<AnalyzedRun> UnicodeAnalyzer::analyze_runs(std::string_view utf8) {
    std::vector<AnalyzedRun> runs;
    if (utf8.empty()) return runs;

    size_t offset = 0;
    TextScript current_script = TextScript::Common;
    TextDirection current_dir = TextDirection::LTR;
    size_t run_start = 0;

    while (offset < utf8.size()) {
        size_t cp_start = offset;
        uint32_t cp = decode_next_codepoint(utf8, offset);
        TextScript script = detect_script(cp);

        // If the script is Common (e.g. space, punctuation), it assumes the current run's script
        if (script != TextScript::Common) {
            TextDirection dir = script_direction(script);
            if (current_script == TextScript::Common) {
                current_script = script;
                current_dir = dir;
            } else if (script != current_script || dir != current_dir) {
                // Emit completed run
                runs.push_back(AnalyzedRun{
                    .script = current_script,
                    .direction = current_dir,
                    .byte_start = run_start,
                    .byte_length = cp_start - run_start,
                    .text = utf8.substr(run_start, cp_start - run_start)
                });
                run_start = cp_start;
                current_script = script;
                current_dir = dir;
            }
        }
    }

    if (run_start < utf8.size()) {
        if (current_script == TextScript::Common) {
            current_script = TextScript::Latin;
        }
        runs.push_back(AnalyzedRun{
            .script = current_script,
            .direction = current_dir,
            .byte_start = run_start,
            .byte_length = utf8.size() - run_start,
            .text = utf8.substr(run_start, utf8.size() - run_start)
        });
    }

    return runs;
}

} // namespace pingdart::text

#pragma once

#include "text_types.h"
#include <string_view>
#include <vector>
#include <cstdint>

namespace pingdart::text {

struct AnalyzedRun {
    TextScript script{TextScript::Common};
    TextDirection direction{TextDirection::LTR};
    size_t byte_start{0};
    size_t byte_length{0};
    std::string_view text;
};

struct GraphemeCluster {
    size_t byte_start{0};
    size_t byte_length{0};
    uint32_t codepoint_count{0};
    bool is_combining_sequence{false};
};

class UnicodeAnalyzer {
public:
    // Decodes the next UTF-8 codepoint starting at byte_offset, advancing byte_offset
    static uint32_t decode_next_codepoint(std::string_view utf8, size_t& byte_offset) noexcept;

    // Detects the script of a Unicode codepoint
    static TextScript detect_script(uint32_t codepoint) noexcept;

    // Determines direction of a script or codepoint
    static TextDirection detect_direction(uint32_t codepoint) noexcept;
    static TextDirection script_direction(TextScript script) noexcept;

    // Checks if a codepoint is a combining mark, vowel sign, or virama
    static bool is_combining_mark(uint32_t codepoint) noexcept;

    // Segments UTF-8 text into grapheme clusters (never splitting combining marks or Indic aksharas)
    static std::vector<GraphemeCluster> segment_grapheme_clusters(std::string_view utf8);

    // Analyzes and segments mixed-language text into homogenous runs of (script, direction)
    static std::vector<AnalyzedRun> analyze_runs(std::string_view utf8);
};

} // namespace pingdart::text

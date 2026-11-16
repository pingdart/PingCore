#include <pingdart/text/text_shaper.h>
#include <pingdart/text/unicode_analyzer.h>
#include <algorithm>

namespace pingdart::text {

TextShaper::TextShaper(FontManager* font_manager)
    : font_manager_(font_manager ? font_manager : &FontManager::instance())
{
}

core::Result<std::vector<ShapedTextRun>> TextShaper::shape_text(
    std::string_view utf8_text,
    const TextStyle& style)
{
    std::vector<ShapedTextRun> shaped_runs;
    if (utf8_text.empty()) {
        return shaped_runs;
    }

    auto analyzed_runs = UnicodeAnalyzer::analyze_runs(utf8_text);

    for (const auto& run : analyzed_runs) {
        size_t sample_offset = 0;
        uint32_t sample_cp = UnicodeAnalyzer::decode_next_codepoint(run.text, sample_offset);

        // Resolve font face (using fallback if primary font doesn't support the script)
        auto font_face = font_manager_->resolve_fallback_font(sample_cp, style);
        if (!font_face) {
            font_face = font_manager_->resolve_font(style);
        }

        FontMetrics metrics = font_face ? font_face->metrics_for_size(style.font_size) : FontMetrics{};

        ShapedTextRun shaped{};
        shaped.font_family = font_face ? font_face->family() : style.font_family;
        shaped.direction = run.direction;
        shaped.script = run.script;
        shaped.ascent = metrics.ascent;
        shaped.descent = metrics.descent;
        shaped.line_height = style.font_size * style.line_height;

        auto clusters = UnicodeAnalyzer::segment_grapheme_clusters(run.text);
        float current_x = 0.0f;

        for (const auto& cl : clusters) {
            size_t cp_off = cl.byte_start;
            uint32_t first_cp = UnicodeAnalyzer::decode_next_codepoint(run.text, cp_off);

            // Compute advance width based on script and character properties
            float advance = style.font_size * 0.55f; // Baseline Latin
            if (run.script == TextScript::Telugu) {
                advance = style.font_size * (cl.is_combining_sequence ? 0.85f : 0.72f);
            } else if (run.script == TextScript::Devanagari) {
                advance = style.font_size * (cl.is_combining_sequence ? 0.80f : 0.68f);
            } else if (run.script == TextScript::Arabic) {
                advance = style.font_size * 0.60f;
            } else if (run.script == TextScript::Emoji) {
                advance = style.font_size * 1.0f;
            } else if (first_cp == 'i' || first_cp == 'l' || first_cp == 't' || first_cp == ' ') {
                advance = style.font_size * 0.30f;
            } else if (first_cp == 'm' || first_cp == 'w' || first_cp == 'M' || first_cp == 'W') {
                advance = style.font_size * 0.85f;
            }
            advance += style.letter_spacing;

            GlyphPosition pos{};
            pos.glyph_id = first_cp;
            pos.x_advance = advance;
            pos.y_advance = 0.0f;
            pos.x_offset = current_x;
            pos.y_offset = 0.0f;
            pos.cluster = static_cast<uint32_t>(run.byte_start + cl.byte_start);

            shaped.glyphs.push_back(pos);
            current_x += advance;
        }

        shaped.width = current_x;

        // For RTL runs (Arabic), mirror x offsets to visual presentation order
        if (shaped.direction == TextDirection::RTL) {
            for (auto& g : shaped.glyphs) {
                g.x_offset = shaped.width - g.x_offset - g.x_advance;
            }
        }

        shaped_runs.push_back(std::move(shaped));
    }

    return shaped_runs;
}

} // namespace pingdart::text

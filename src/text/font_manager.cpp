#include <pingdart/text/font_manager.h>
#include <pingdart/text/unicode_analyzer.h>
#include <algorithm>

namespace pingdart::text {

FontFace::FontFace(
    std::string family,
    FontWeight weight,
    FontStyle style,
    std::vector<TextScript> supported_scripts)
    : family_(std::move(family))
    , weight_(weight)
    , style_(style)
    , supported_scripts_(std::move(supported_scripts))
{
}

bool FontFace::supports_script(TextScript script) const noexcept {
    if (script == TextScript::Common) return true;
    for (auto s : supported_scripts_) {
        if (s == script) return true;
    }
    return false;
}

bool FontFace::supports_codepoint(uint32_t codepoint) const noexcept {
    TextScript script = UnicodeAnalyzer::detect_script(codepoint);
    return supports_script(script);
}

FontMetrics FontFace::metrics_for_size(float font_size) const noexcept {
    FontMetrics m{};
    m.ascent = font_size * 0.82f;
    m.descent = font_size * 0.22f;
    m.cap_height = font_size * 0.70f;
    m.x_height = font_size * 0.52f;
    m.leading = font_size * 0.16f;
    m.underline_position = font_size * 0.12f;
    m.underline_thickness = std::max(1.0f, font_size * 0.08f);
    return m;
}

FontManager::FontManager() {
    register_defaults();
}

void FontManager::register_defaults() {
    // 1. Latin primary UI font
    register_font_face(std::make_shared<FontFace>(
        "Roboto",
        FontWeight::Normal,
        FontStyle::Normal,
        std::vector<TextScript>{TextScript::Latin, TextScript::Common}
    ));
    register_font_face(std::make_shared<FontFace>(
        "Roboto",
        FontWeight::Bold,
        FontStyle::Normal,
        std::vector<TextScript>{TextScript::Latin, TextScript::Common}
    ));

    // 2. Telugu script font (for internationalized UI)
    register_font_face(std::make_shared<FontFace>(
        "Noto Sans Telugu",
        FontWeight::Normal,
        FontStyle::Normal,
        std::vector<TextScript>{TextScript::Telugu, TextScript::Common}
    ));

    // 3. Devanagari (Hindi, Marathi, Sanskrit)
    register_font_face(std::make_shared<FontFace>(
        "Noto Sans Devanagari",
        FontWeight::Normal,
        FontStyle::Normal,
        std::vector<TextScript>{TextScript::Devanagari, TextScript::Common}
    ));

    // 4. Arabic script font (RTL)
    register_font_face(std::make_shared<FontFace>(
        "Noto Sans Arabic",
        FontWeight::Normal,
        FontStyle::Normal,
        std::vector<TextScript>{TextScript::Arabic, TextScript::Common}
    ));

    // 5. Emoji font
    register_font_face(std::make_shared<FontFace>(
        "Noto Color Emoji",
        FontWeight::Normal,
        FontStyle::Normal,
        std::vector<TextScript>{TextScript::Emoji}
    ));
}

void FontManager::register_font_face(std::shared_ptr<FontFace> face) {
    font_faces_.push_back(std::move(face));
}

std::shared_ptr<FontFace> FontManager::resolve_font(const TextStyle& style) const {
    FontWeight target_weight = static_cast<FontWeight>(style.font_weight);

    // Exact match (family, weight, style)
    for (const auto& face : font_faces_) {
        if (face->family() == style.font_family &&
            face->weight() == target_weight &&
            face->style() == style.font_style) {
            return face;
        }
    }

    // Family match with any weight
    for (const auto& face : font_faces_) {
        if (face->family() == style.font_family) {
            return face;
        }
    }

    // Default UI font fallback
    return font_faces_.empty() ? nullptr : font_faces_[0];
}

std::shared_ptr<FontFace> FontManager::resolve_fallback_font(
    uint32_t codepoint,
    const TextStyle& preferred_style) const
{
    auto primary = resolve_font(preferred_style);
    if (primary && primary->supports_codepoint(codepoint)) {
        return primary;
    }

    TextScript needed_script = UnicodeAnalyzer::detect_script(codepoint);
    for (const auto& face : font_faces_) {
        if (face->supports_script(needed_script)) {
            return face;
        }
    }

    return primary;
}

FontManager& FontManager::instance() noexcept {
    static FontManager instance_;
    return instance_;
}

} // namespace pingdart::text

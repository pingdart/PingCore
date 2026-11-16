#pragma once

#include "text_types.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace pingdart::text {

class FontFace {
public:
    FontFace(std::string family, FontWeight weight, FontStyle style, std::vector<TextScript> supported_scripts);
    virtual ~FontFace() = default;

    [[nodiscard]] const std::string& family() const noexcept { return family_; }
    [[nodiscard]] FontWeight weight() const noexcept { return weight_; }
    [[nodiscard]] FontStyle style() const noexcept { return style_; }

    [[nodiscard]] bool supports_script(TextScript script) const noexcept;
    [[nodiscard]] bool supports_codepoint(uint32_t codepoint) const noexcept;

    [[nodiscard]] FontMetrics metrics_for_size(float font_size) const noexcept;

private:
    std::string family_;
    FontWeight weight_;
    FontStyle style_;
    std::vector<TextScript> supported_scripts_;
};

class FontManager {
public:
    FontManager();
    virtual ~FontManager() = default;

    void register_font_face(std::shared_ptr<FontFace> face);

    // Resolves primary font face for a given style
    [[nodiscard]] std::shared_ptr<FontFace> resolve_font(const TextStyle& style) const;

    // Resolves fallback font face if primary font lacks support for a codepoint
    [[nodiscard]] std::shared_ptr<FontFace> resolve_fallback_font(
        uint32_t codepoint,
        const TextStyle& preferred_style) const;

    static FontManager& instance() noexcept;

private:
    void register_defaults();

    std::vector<std::shared_ptr<FontFace>> font_faces_;
};

} // namespace pingdart::text

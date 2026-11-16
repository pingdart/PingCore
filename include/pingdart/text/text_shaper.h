#pragma once

#include "text_types.h"
#include "font_manager.h"
#include <pingdart/core/result.h>
#include <memory>
#include <string_view>

namespace pingdart::text {

class ITextShaper {
public:
    virtual ~ITextShaper() = default;

    virtual core::Result<std::vector<ShapedTextRun>> shape_text(
        std::string_view utf8_text,
        const TextStyle& style
    ) = 0;
};

class TextShaper final : public ITextShaper {
public:
    explicit TextShaper(FontManager* font_manager = nullptr);
    ~TextShaper() override = default;

    core::Result<std::vector<ShapedTextRun>> shape_text(
        std::string_view utf8_text,
        const TextStyle& style
    ) override;

private:
    FontManager* font_manager_{nullptr};
};

} // namespace pingdart::text

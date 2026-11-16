#pragma once

#include "text_types.h"
#include "text_shaper.h"
#include <pingdart/rhi/display_list.h>
#include <pingdart/rhi/geometry.h>
#include <string>
#include <vector>
#include <memory>

namespace pingdart::text {

enum class TextAlign : uint8_t {
    Left,
    Center,
    Right,
    Justify
};

struct ParagraphStyle {
    TextStyle default_style{};
    TextAlign align{TextAlign::Left};
    uint32_t max_lines{0}; // 0 = unlimited
    std::string ellipsis{};
};

struct TextPosition {
    size_t offset{0};
    rhi::Point caret_point{};
    float line_height{0.0f};
};

struct LineMetrics {
    size_t run_start{0};
    size_t run_end{0};
    float width{0.0f};
    float height{0.0f};
    float baseline{0.0f};
    rhi::Point origin{};
};

class Paragraph {
public:
    Paragraph(std::string text, ParagraphStyle style, std::shared_ptr<ITextShaper> shaper = nullptr);
    ~Paragraph() = default;

    // Lays out lines within constraint width
    void layout(float max_width);

    // Queries metrics
    [[nodiscard]] float width() const noexcept { return width_; }
    [[nodiscard]] float height() const noexcept { return height_; }
    [[nodiscard]] size_t line_count() const noexcept { return lines_.size(); }
    [[nodiscard]] const std::vector<LineMetrics>& lines() const noexcept { return lines_; }

    // Hit-testing & Caret mapping
    [[nodiscard]] TextPosition get_position_for_offset(size_t utf8_offset) const noexcept;
    [[nodiscard]] size_t get_offset_for_position(rhi::Point point) const noexcept;

    // Emits shaped text runs into DisplayList
    void paint(rhi::DisplayListBuilder& builder, rhi::Point origin) const;

private:
    std::string text_;
    ParagraphStyle style_;
    std::shared_ptr<ITextShaper> shaper_;

    std::vector<ShapedTextRun> shaped_runs_;
    std::vector<LineMetrics> lines_;
    float width_{0.0f};
    float height_{0.0f};
};

class ParagraphBuilder {
public:
    explicit ParagraphBuilder(ParagraphStyle style, std::shared_ptr<ITextShaper> shaper = nullptr);

    void add_text(std::string_view text);
    [[nodiscard]] std::unique_ptr<Paragraph> build();

private:
    ParagraphStyle style_;
    std::shared_ptr<ITextShaper> shaper_;
    std::string text_;
};

} // namespace pingdart::text

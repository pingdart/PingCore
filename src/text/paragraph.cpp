#include <pingdart/text/paragraph.h>
#include <algorithm>

namespace pingdart::text {

Paragraph::Paragraph(std::string text, ParagraphStyle style, std::shared_ptr<ITextShaper> shaper)
    : text_(std::move(text))
    , style_(std::move(style))
    , shaper_(shaper ? shaper : std::make_shared<TextShaper>())
{
}

void Paragraph::layout(float max_width) {
    lines_.clear();
    width_ = 0.0f;
    height_ = 0.0f;

    if (text_.empty()) {
        return;
    }

    auto shape_res = shaper_->shape_text(text_, style_.default_style);
    if (!shape_res.has_value()) {
        return;
    }
    shaped_runs_ = std::move(shape_res).value();

    float current_y = 0.0f;
    float max_line_w = 0.0f;

    // Line breaking & alignment
    for (size_t run_idx = 0; run_idx < shaped_runs_.size(); ++run_idx) {
        const auto& run = shaped_runs_[run_idx];
        float line_h = run.line_height > 0.0f ? run.line_height : style_.default_style.font_size * 1.2f;

        float current_line_width = 0.0f;
        size_t line_start_glyph = 0;

        for (size_t g_idx = 0; g_idx < run.glyphs.size(); ++g_idx) {
            float adv = run.glyphs[g_idx].x_advance;

            if (max_width > 0.0f && (current_line_width + adv > max_width) && g_idx > line_start_glyph) {
                // Line break
                float align_x = 0.0f;
                if (style_.align == TextAlign::Center) {
                    align_x = std::max(0.0f, (max_width - current_line_width) * 0.5f);
                } else if (style_.align == TextAlign::Right) {
                    align_x = std::max(0.0f, max_width - current_line_width);
                }

                lines_.push_back(LineMetrics{
                    .run_start = line_start_glyph,
                    .run_end = g_idx,
                    .width = current_line_width,
                    .height = line_h,
                    .baseline = run.ascent,
                    .origin = rhi::Point{align_x, current_y}
                });

                max_line_w = std::max(max_line_w, current_line_width);
                current_y += line_h;
                current_line_width = 0.0f;
                line_start_glyph = g_idx;

                if (style_.max_lines > 0 && lines_.size() >= style_.max_lines) {
                    break;
                }
            }

            current_line_width += adv;
        }

        if (line_start_glyph < run.glyphs.size() &&
            (style_.max_lines == 0 || lines_.size() < style_.max_lines)) {
            float align_x = 0.0f;
            if (style_.align == TextAlign::Center && max_width > 0.0f) {
                align_x = std::max(0.0f, (max_width - current_line_width) * 0.5f);
            } else if (style_.align == TextAlign::Right && max_width > 0.0f) {
                align_x = std::max(0.0f, max_width - current_line_width);
            }

            lines_.push_back(LineMetrics{
                .run_start = line_start_glyph,
                .run_end = run.glyphs.size(),
                .width = current_line_width,
                .height = line_h,
                .baseline = run.ascent,
                .origin = rhi::Point{align_x, current_y}
            });

            max_line_w = std::max(max_line_w, current_line_width);
            current_y += line_h;
        }
    }

    width_ = max_line_w;
    height_ = current_y;
}

TextPosition Paragraph::get_position_for_offset(size_t utf8_offset) const noexcept {
    if (shaped_runs_.empty() || lines_.empty()) {
        return TextPosition{0, rhi::Point{0.0f, 0.0f}, style_.default_style.font_size * 1.2f};
    }

    for (const auto& line : lines_) {
        const auto& run = shaped_runs_[0];
        for (size_t i = line.run_start; i < line.run_end; ++i) {
            if (run.glyphs[i].cluster >= utf8_offset) {
                float glyph_rel_x = run.glyphs[i].x_offset;
                return TextPosition{
                    .offset = utf8_offset,
                    .caret_point = rhi::Point{line.origin.x + glyph_rel_x, line.origin.y},
                    .line_height = line.height
                };
            }
        }
    }

    // Offset is at the end of the paragraph
    const auto& last_line = lines_.back();
    return TextPosition{
        .offset = utf8_offset,
        .caret_point = rhi::Point{last_line.origin.x + last_line.width, last_line.origin.y},
        .line_height = last_line.height
    };
}

size_t Paragraph::get_offset_for_position(rhi::Point point) const noexcept {
    if (lines_.empty() || shaped_runs_.empty()) {
        return 0;
    }

    // Find the line matching point.y
    const LineMetrics* target_line = &lines_[0];
    for (const auto& line : lines_) {
        if (point.y >= line.origin.y && point.y <= line.origin.y + line.height) {
            target_line = &line;
            break;
        }
    }

    const auto& run = shaped_runs_[0];
    size_t closest_cluster = 0;
    float closest_dist = 1e9f;

    for (size_t i = target_line->run_start; i < target_line->run_end; ++i) {
        float gx = target_line->origin.x + run.glyphs[i].x_offset;
        float dist = std::abs(point.x - gx);
        if (dist < closest_dist) {
            closest_dist = dist;
            closest_cluster = run.glyphs[i].cluster;
        }
    }

    return closest_cluster;
}

void Paragraph::paint(rhi::DisplayListBuilder& builder, rhi::Point origin) const {
    if (shaped_runs_.empty() || lines_.empty()) {
        return;
    }

    rhi::Paint paint{};
    paint.color = style_.default_style.color;

    for (const auto& line : lines_) {
        const auto& run = shaped_runs_[0];
        rhi::Point line_origin{origin.x + line.origin.x, origin.y + line.origin.y + line.baseline};

        rhi::TextBlob blob{};
        blob.text = text_.substr(
            run.glyphs[line.run_start].cluster,
            (line.run_end < run.glyphs.size() ? run.glyphs[line.run_end].cluster : text_.size()) - run.glyphs[line.run_start].cluster
        );
        blob.font_family = run.font_family;
        blob.font_size = style_.default_style.font_size;
        blob.bold = (style_.default_style.font_weight >= 700);

        builder.draw_text_blob(std::move(blob), line_origin, paint);
    }
}

ParagraphBuilder::ParagraphBuilder(ParagraphStyle style, std::shared_ptr<ITextShaper> shaper)
    : style_(std::move(style))
    , shaper_(std::move(shaper))
{
}

void ParagraphBuilder::add_text(std::string_view text) {
    text_.append(text);
}

std::unique_ptr<Paragraph> ParagraphBuilder::build() {
    return std::make_unique<Paragraph>(std::move(text_), style_, shaper_);
}

} // namespace pingdart::text

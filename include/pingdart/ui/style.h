// include/pingdart/ui/style.h
// PingDartUI Production Styling & Cascade Engine (PKG-136)
#pragma once

#include <pingdart/ui/theme.h>
#include <pingdart/ui/css_parser.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <optional>
#include <mutex>

namespace pingdart::ui {

enum class FontWeight {
    Normal = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700
};

enum class TextAlign {
    Left,
    Center,
    Right,
    Justify
};

enum class FlexDirection {
    Column,
    Row,
    ColumnReverse,
    RowReverse
};

enum class FlexWrap {
    NoWrap,
    Wrap,
    WrapReverse
};

enum class AlignItems {
    Start,
    Center,
    End,
    Stretch,
    Baseline
};

enum class JustifyContent {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

enum class ButtonVariant {
    Default,
    Primary,
    Secondary,
    Outline,
    Ghost,
    Destructive
};

enum class Breakpoint {
    Base,
    Sm,  // 640px
    Md,  // 768px
    Lg,  // 1024px
    Xl,  // 1280px
    Xxl  // 1536px
};

struct Insets {
    float top{0};
    float right{0};
    float bottom{0};
    float left{0};

    Insets() = default;
    Insets(float all) : top(all), right(all), bottom(all), left(all) {}
    Insets(float v, float h) : top(v), right(h), bottom(v), left(h) {}
    Insets(float t, float r, float b, float l) : top(t), right(r), bottom(b), left(l) {}
};

struct BoxShadow {
    float x{0};
    float y{0};
    float blur{0};
    float spread{0};
    std::string color{""};
};

// Unified ComputedStyle representation across CSS, Tailwind, & Fluent C++
struct ComputedStyle {
    // Visuals
    std::string background_color{""};
    std::string text_color{""};
    float opacity{1.0f};

    // Border
    std::string border_color{""};
    float border_width{0.0f};
    float border_radius{0.0f};
    std::string border_style{"solid"};

    // Box Model
    Insets padding{};
    Insets margin{};

    // Layout & Display
    std::string display{""};
    std::string position{""};
    std::string overflow{""};
    std::string cursor{""};

    // Sizing
    std::string width{""};
    std::string height{""};
    float min_width{0.0f};
    float max_width{0.0f};
    float min_height{0.0f};
    float max_height{0.0f};

    // Flexbox
    bool is_flex{true};
    FlexDirection flex_direction{FlexDirection::Column};
    FlexWrap flex_wrap{FlexWrap::NoWrap};
    AlignItems align_items{AlignItems::Start};
    JustifyContent justify_content{JustifyContent::Start};
    float flex_grow{0.0f};
    float flex_shrink{1.0f};
    float gap{0.0f};

    // Grid
    bool is_grid{false};
    std::string grid_template_columns{""};
    std::string grid_template_rows{""};
    float grid_gap{0.0f};

    // Typography
    std::string font_family{""};
    float font_size{16.0f};
    FontWeight font_weight{FontWeight::Normal};
    float line_height{1.5f};
    float letter_spacing{0.0f};
    TextAlign text_align{TextAlign::Left};

    // Effects & Shadow
    BoxShadow shadow{};

    // Variants & Metadata
    ButtonVariant variant{ButtonVariant::Default};
    std::vector<std::string> class_names{};

    // Important Flags for Cascade Overrides
    struct ImportantFlags {
        bool background_color{false};
        bool text_color{false};
        bool padding{false};
        bool margin{false};
        bool border{false};
        bool border_radius{false};
        bool width{false};
        bool height{false};
        bool font_size{false};
        bool gap{false};
    } important{};

    // State Variants
    std::shared_ptr<ComputedStyle> hover_style{nullptr};
    std::shared_ptr<ComputedStyle> focus_style{nullptr};
    std::shared_ptr<ComputedStyle> active_style{nullptr};
    std::shared_ptr<ComputedStyle> disabled_style{nullptr};

    std::string to_css() const {
        std::ostringstream ss;
        if (!background_color.empty()) ss << "background-color: " << background_color << "; ";
        if (!text_color.empty()) ss << "color: " << text_color << "; ";
        if (opacity < 1.0f && opacity >= 0.0f) ss << "opacity: " << opacity << "; ";
        if (!border_color.empty() && border_width > 0) {
            ss << "border: " << border_width << "px " << border_style << " " << border_color << "; ";
        }
        if (border_radius > 0) ss << "border-radius: " << border_radius << "px; ";
        if (!font_family.empty()) ss << "font-family: " << font_family << "; ";
        if (font_size > 0) ss << "font-size: " << font_size << "px; ";
        if (font_weight != FontWeight::Normal) {
            ss << "font-weight: " << static_cast<int>(font_weight) << "; ";
        }
        if (line_height != 1.5f && line_height > 0) {
            ss << "line-height: " << line_height << "; ";
        }
        if (letter_spacing != 0.0f) {
            ss << "letter-spacing: " << letter_spacing << "px; ";
        }
        if (padding.top > 0 || padding.right > 0 || padding.bottom > 0 || padding.left > 0) {
            ss << "padding: " << padding.top << "px " << padding.right << "px " 
               << padding.bottom << "px " << padding.left << "px; ";
        }
        if (margin.top > 0 || margin.right > 0 || margin.bottom > 0 || margin.left > 0) {
            ss << "margin: " << margin.top << "px " << margin.right << "px " 
               << margin.bottom << "px " << margin.left << "px; ";
        }
        if (is_grid) {
            ss << "display: grid; ";
            if (!grid_template_columns.empty()) ss << "grid-template-columns: " << grid_template_columns << "; ";
            if (grid_gap > 0) ss << "gap: " << grid_gap << "px; ";
        } else if (is_flex) {
            if (flex_direction == FlexDirection::Row) ss << "display: flex; flex-direction: row; ";
            else if (flex_direction == FlexDirection::Column) ss << "display: flex; flex-direction: column; ";
            else if (flex_direction == FlexDirection::RowReverse) ss << "display: flex; flex-direction: row-reverse; ";
            else if (flex_direction == FlexDirection::ColumnReverse) ss << "display: flex; flex-direction: column-reverse; ";

            if (flex_wrap == FlexWrap::Wrap) ss << "flex-wrap: wrap; ";
            if (gap > 0) ss << "gap: " << gap << "px; ";
            if (align_items == AlignItems::Center) ss << "align-items: center; ";
            else if (align_items == AlignItems::End) ss << "align-items: flex-end; ";
            else if (align_items == AlignItems::Stretch) ss << "align-items: stretch; ";

            if (justify_content == JustifyContent::Center) ss << "justify-content: center; ";
            else if (justify_content == JustifyContent::SpaceBetween) ss << "justify-content: space-between; ";
            else if (justify_content == JustifyContent::End) ss << "justify-content: flex-end; ";
        }
        if (!display.empty()) ss << "display: " << display << "; ";
        if (!position.empty()) ss << "position: " << position << "; ";
        if (!overflow.empty()) ss << "overflow: " << overflow << "; ";
        if (!cursor.empty()) ss << "cursor: " << cursor << "; ";
        if (!width.empty()) ss << "width: " << width << "; ";
        if (!height.empty()) ss << "height: " << height << "; ";
        if (max_width > 0) ss << "max-width: " << max_width << "px; ";
        if (min_width > 0) ss << "min-width: " << min_width << "px; ";
        if (max_height > 0) ss << "max-height: " << max_height << "px; ";
        if (min_height > 0) ss << "min-height: " << min_height << "px; ";
        if (!shadow.color.empty() && (shadow.x != 0 || shadow.y != 0 || shadow.blur != 0)) {
            ss << "box-shadow: " << shadow.x << "px " << shadow.y << "px " 
               << shadow.blur << "px " << shadow.color << "; ";
        }
        return ss.str();
    }
};

// Fast Utility-Class Cache Engine
class UtilityClassCompiler {
public:
    static UtilityClassCompiler& instance() {
        static UtilityClassCompiler s_inst;
        return s_inst;
    }

    ComputedStyle compile_classes(const std::string& class_names) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(class_names);
        if (it != cache_.end()) {
            return it->second;
        }

        ComputedStyle style;
        std::istringstream iss(class_names);
        std::string token;
        auto& theme = ThemeEngine::instance();

        while (iss >> token) {
            style.class_names.push_back(token);
            apply_token(style, token, theme);
        }

        cache_[class_names] = style;
        return style;
    }

private:
    void apply_token(ComputedStyle& s, const std::string& token, ThemeEngine& theme) {
        // Colors from bundled palettes: e.g. bg-blue-600, text-slate-800
        if (token.starts_with("bg-")) {
            std::string rem = token.substr(3);
            if (rem == "white") s.background_color = "#FFFFFF";
            else if (rem == "black") s.background_color = "#000000";
            else if (rem == "primary") s.background_color = theme.tokens().primary;
            else if (rem == "secondary") s.background_color = theme.tokens().secondary;
            else {
                size_t dash = rem.rfind('-');
                if (dash != std::string::npos) {
                    std::string pal = rem.substr(0, dash);
                    try {
                        int shade = std::stoi(rem.substr(dash + 1));
                        std::string hex = theme.get_color(pal, shade);
                        if (!hex.empty()) s.background_color = hex;
                    } catch (...) {}
                }
            }
        } else if (token.starts_with("text-")) {
            std::string rem = token.substr(5);
            if (rem == "white") s.text_color = "#FFFFFF";
            else if (rem == "black") s.text_color = "#000000";
            else if (rem == "primary") s.text_color = theme.tokens().primary;
            else if (rem == "secondary") s.text_color = theme.tokens().secondary;
            else if (rem == "sm") s.font_size = 14;
            else if (rem == "base") s.font_size = 16;
            else if (rem == "lg") s.font_size = 18;
            else if (rem == "xl") s.font_size = 20;
            else if (rem == "2xl") s.font_size = 24;
            else if (rem == "3xl") s.font_size = 30;
            else {
                size_t dash = rem.rfind('-');
                if (dash != std::string::npos) {
                    std::string pal = rem.substr(0, dash);
                    try {
                        int shade = std::stoi(rem.substr(dash + 1));
                        std::string hex = theme.get_color(pal, shade);
                        if (!hex.empty()) s.text_color = hex;
                    } catch (...) {}
                }
            }
        } else if (token.starts_with("p-")) {
            float val = std::stof(token.substr(2)) * 4.0f;
            s.padding = Insets(val);
        } else if (token.starts_with("px-")) {
            float val = std::stof(token.substr(3)) * 4.0f;
            s.padding.left = s.padding.right = val;
        } else if (token.starts_with("py-")) {
            float val = std::stof(token.substr(3)) * 4.0f;
            s.padding.top = s.padding.bottom = val;
        } else if (token.starts_with("m-")) {
            float val = std::stof(token.substr(2)) * 4.0f;
            s.margin = Insets(val);
        } else if (token.starts_with("gap-")) {
            s.gap = std::stof(token.substr(4)) * 4.0f;
        } else if (token.starts_with("rounded-")) {
            std::string r = token.substr(8);
            if (r == "none") s.border_radius = 0;
            else if (r == "sm") s.border_radius = 4;
            else if (r == "md") s.border_radius = 6;
            else if (r == "lg") s.border_radius = 8;
            else if (r == "xl") s.border_radius = 12;
            else if (r == "2xl") s.border_radius = 16;
            else if (r == "full") s.border_radius = 9999;
        } else if (token == "flex-row") {
            s.flex_direction = FlexDirection::Row;
        } else if (token == "flex-col") {
            s.flex_direction = FlexDirection::Column;
        } else if (token == "items-center") {
            s.align_items = AlignItems::Center;
        } else if (token == "justify-between") {
            s.justify_content = JustifyContent::SpaceBetween;
        } else if (token == "justify-center") {
            s.justify_content = JustifyContent::Center;
        } else if (token == "w-full") {
            s.width = "100%";
        } else if (token == "h-full") {
            s.height = "100%";
        } else if (token == "font-bold") {
            s.font_weight = FontWeight::Bold;
        } else if (token == "font-medium") {
            s.font_weight = FontWeight::Medium;
        } else if (token == "font-semibold") {
            s.font_weight = FontWeight::SemiBold;
        } else if (token == "shadow-sm") {
            s.shadow = BoxShadow{0, 1, 2, 0, "#0000000D"};
        } else if (token == "shadow-md") {
            s.shadow = BoxShadow{0, 4, 6, -1, "#0000001A"};
        } else if (token == "shadow-lg") {
            s.shadow = BoxShadow{0, 10, 15, -3, "#00000026"};
        } else if (token == "cursor-pointer") {
            s.cursor = "pointer";
        } else if (token == "overflow-hidden") {
            s.overflow = "hidden";
        } else if (token == "overflow-auto") {
            s.overflow = "auto";
        } else if (token == "overflow-y-auto") {
            s.overflow = "auto";
        } else if (token == "relative") {
            s.position = "relative";
        } else if (token == "absolute") {
            s.position = "absolute";
        } else if (token == "grid") {
            s.is_grid = true;
            s.is_flex = false;
        } else if (token == "flex") {
            s.is_flex = true;
            s.is_grid = false;
        } else if (token == "block") {
            s.display = "block";
            s.is_flex = false;
        }
    }

    std::mutex mutex_{};
    std::unordered_map<std::string, ComputedStyle> cache_{};
};

inline void apply_utility_classes(ComputedStyle& s, const std::string& class_names) {
    auto parsed = UtilityClassCompiler::instance().compile_classes(class_names);
    if (!parsed.background_color.empty()) s.background_color = parsed.background_color;
    if (!parsed.text_color.empty()) s.text_color = parsed.text_color;
    if (parsed.border_radius > 0) s.border_radius = parsed.border_radius;
    if (parsed.padding.top > 0 || parsed.padding.left > 0) s.padding = parsed.padding;
    if (parsed.margin.top > 0 || parsed.margin.left > 0) s.margin = parsed.margin;
    if (parsed.gap > 0) s.gap = parsed.gap;
    if (parsed.font_size != 16.0f) s.font_size = parsed.font_size;
    if (parsed.font_weight != FontWeight::Normal) s.font_weight = parsed.font_weight;
    if (!parsed.width.empty()) s.width = parsed.width;
    if (!parsed.height.empty()) s.height = parsed.height;
    if (parsed.flex_direction == FlexDirection::Row) s.flex_direction = FlexDirection::Row;
    if (parsed.align_items == AlignItems::Center) s.align_items = AlignItems::Center;
    if (parsed.justify_content != JustifyContent::Start) s.justify_content = parsed.justify_content;
    if (!parsed.position.empty()) s.position = parsed.position;
    if (!parsed.overflow.empty()) s.overflow = parsed.overflow;
    if (!parsed.cursor.empty()) s.cursor = parsed.cursor;
    if (!parsed.display.empty()) s.display = parsed.display;
    if (parsed.is_grid) { s.is_grid = true; s.is_flex = false; }
    if (!parsed.shadow.color.empty()) s.shadow = parsed.shadow;
    s.class_names.insert(s.class_names.end(), parsed.class_names.begin(), parsed.class_names.end());
}

// Fluent Style Builder CRTP
template <typename Derived>
class StyleBuilder {
protected:
    ComputedStyle style_{};

public:
    Derived& background(const std::string& color) {
        style_.background_color = color;
        return static_cast<Derived&>(*this);
    }

    Derived& color(const std::string& color) {
        style_.text_color = color;
        return static_cast<Derived&>(*this);
    }

    Derived& opacity(float op) {
        style_.opacity = op;
        return static_cast<Derived&>(*this);
    }

    Derived& border(float width, const std::string& color, const std::string& style = "solid") {
        style_.border_width = width;
        style_.border_color = color;
        style_.border_style = style;
        return static_cast<Derived&>(*this);
    }

    Derived& borderRadius(float radius) {
        style_.border_radius = radius;
        return static_cast<Derived&>(*this);
    }

    Derived& padding(float all) {
        style_.padding = Insets(all);
        return static_cast<Derived&>(*this);
    }

    Derived& padding(float v, float h) {
        style_.padding = Insets(v, h);
        return static_cast<Derived&>(*this);
    }

    Derived& padding(float t, float r, float b, float l) {
        style_.padding = Insets(t, r, b, l);
        return static_cast<Derived&>(*this);
    }

    Derived& margin(float all) {
        style_.margin = Insets(all);
        return static_cast<Derived&>(*this);
    }

    Derived& margin(float v, float h) {
        style_.margin = Insets(v, h);
        return static_cast<Derived&>(*this);
    }

    Derived& margin(float t, float r, float b, float l) {
        style_.margin = Insets(t, r, b, l);
        return static_cast<Derived&>(*this);
    }

    Derived& fontSize(float size) {
        style_.font_size = size;
        return static_cast<Derived&>(*this);
    }

    Derived& size(float size) {
        return fontSize(size);
    }

    Derived& weight(FontWeight w) {
        style_.font_weight = w;
        return static_cast<Derived&>(*this);
    }

    Derived& fontFamily(const std::string& family) {
        style_.font_family = family;
        return static_cast<Derived&>(*this);
    }

    Derived& lineHeight(float lh) {
        style_.line_height = lh;
        return static_cast<Derived&>(*this);
    }

    Derived& textAlign(TextAlign align) {
        style_.text_align = align;
        return static_cast<Derived&>(*this);
    }

    Derived& gap(float g) {
        style_.gap = g;
        return static_cast<Derived&>(*this);
    }

    Derived& width(const std::string& w) {
        style_.width = w;
        return static_cast<Derived&>(*this);
    }

    Derived& width(float w) {
        style_.width = std::to_string(static_cast<int>(w)) + "px";
        return static_cast<Derived&>(*this);
    }

    Derived& height(const std::string& h) {
        style_.height = h;
        return static_cast<Derived&>(*this);
    }

    Derived& height(float h) {
        style_.height = std::to_string(static_cast<int>(h)) + "px";
        return static_cast<Derived&>(*this);
    }

    Derived& maxWidth(float mw) {
        style_.max_width = mw;
        return static_cast<Derived&>(*this);
    }

    Derived& minWidth(float mw) {
        style_.min_width = mw;
        return static_cast<Derived&>(*this);
    }

    Derived& maxHeight(float mh) {
        style_.max_height = mh;
        return static_cast<Derived&>(*this);
    }

    Derived& minHeight(float mh) {
        style_.min_height = mh;
        return static_cast<Derived&>(*this);
    }

    Derived& shadow(float x, float y, float blur, const std::string& col) {
        style_.shadow = BoxShadow{x, y, blur, 0, col};
        return static_cast<Derived&>(*this);
    }

    Derived& shadow(float x, float y, float blur, float spread, const std::string& col) {
        style_.shadow = BoxShadow{x, y, blur, spread, col};
        return static_cast<Derived&>(*this);
    }

    Derived& alignItems(AlignItems ai) {
        style_.align_items = ai;
        return static_cast<Derived&>(*this);
    }

    Derived& justifyContent(JustifyContent jc) {
        style_.justify_content = jc;
        return static_cast<Derived&>(*this);
    }

    Derived& flexDirection(FlexDirection fd) {
        style_.flex_direction = fd;
        return static_cast<Derived&>(*this);
    }

    Derived& flexWrap(FlexWrap fw) {
        style_.flex_wrap = fw;
        return static_cast<Derived&>(*this);
    }

    Derived& grid(const std::string& cols, float gap_px = 0.0f) {
        style_.is_grid = true;
        style_.is_flex = false;
        style_.grid_template_columns = cols;
        style_.grid_gap = gap_px;
        return static_cast<Derived&>(*this);
    }

    Derived& position(const std::string& pos) {
        style_.position = pos;
        return static_cast<Derived&>(*this);
    }

    Derived& overflow(const std::string& ov) {
        style_.overflow = ov;
        return static_cast<Derived&>(*this);
    }

    Derived& overflowY(const std::string& ov) {
        style_.overflow = ov;
        return static_cast<Derived&>(*this);
    }

    Derived& cursor(const std::string& cur) {
        style_.cursor = cur;
        return static_cast<Derived&>(*this);
    }

    Derived& display(const std::string& disp) {
        style_.display = disp;
        return static_cast<Derived&>(*this);
    }

    Derived& variant(ButtonVariant v) {
        style_.variant = v;
        auto& theme = ThemeEngine::instance();
        if (v == ButtonVariant::Primary) {
            style_.background_color = theme.tokens().primary;
            style_.text_color = "#FFFFFF";
        } else if (v == ButtonVariant::Secondary) {
            style_.background_color = theme.tokens().muted;
            style_.text_color = theme.tokens().foreground;
        } else if (v == ButtonVariant::Destructive) {
            style_.background_color = theme.tokens().danger;
            style_.text_color = "#FFFFFF";
        }
        return static_cast<Derived&>(*this);
    }

    Derived& variant(const std::string& v) {
        if (v == "primary") return variant(ButtonVariant::Primary);
        if (v == "secondary") return variant(ButtonVariant::Secondary);
        if (v == "destructive") return variant(ButtonVariant::Destructive);
        if (v == "outline") return variant(ButtonVariant::Outline);
        if (v == "ghost") return variant(ButtonVariant::Ghost);
        return variant(ButtonVariant::Default);
    }

    Derived& className(const std::string& cls) {
        apply_utility_classes(style_, cls);
        return static_cast<Derived&>(*this);
    }

    const ComputedStyle& computed_style() const {
        return style_;
    }

    ComputedStyle& computed_style() {
        return style_;
    }
};

} // namespace pingdart::ui

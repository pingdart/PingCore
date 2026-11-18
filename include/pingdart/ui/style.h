// include/pingdart/ui/style.h
// PingDartUI Unified Styling System: CSS, Tailwind, & Fluent C++ (PKG-135)
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <cstdint>
#include <algorithm>
#include <optional>

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

enum class AlignItems {
    Start,
    Center,
    End,
    Stretch
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
    std::string color{"#00000000"};
};

struct ComputedStyle {
    std::string background_color{""};
    std::string text_color{""};
    std::string border_color{""};
    float border_width{0.0f};
    float border_radius{0.0f};
    float font_size{16.0f};
    FontWeight font_weight{FontWeight::Normal};
    TextAlign text_align{TextAlign::Left};
    Insets padding{};
    Insets margin{};
    FlexDirection flex_direction{FlexDirection::Column};
    AlignItems align_items{AlignItems::Start};
    JustifyContent justify_content{JustifyContent::Start};
    float gap{0.0f};
    std::string width{""};
    std::string height{""};
    float max_width{0.0f};
    float min_height{0.0f};
    BoxShadow shadow{};
    ButtonVariant variant{ButtonVariant::Default};
    std::vector<std::string> class_names{};

    std::string to_css() const {
        std::ostringstream ss;
        if (!background_color.empty()) ss << "background-color: " << background_color << "; ";
        if (!text_color.empty()) ss << "color: " << text_color << "; ";
        if (!border_color.empty() && border_width > 0) {
            ss << "border: " << border_width << "px solid " << border_color << "; ";
        }
        if (border_radius > 0) ss << "border-radius: " << border_radius << "px; ";
        if (font_size > 0) ss << "font-size: " << font_size << "px; ";
        if (font_weight != FontWeight::Normal) {
            ss << "font-weight: " << static_cast<int>(font_weight) << "; ";
        }
        if (padding.top > 0 || padding.right > 0 || padding.bottom > 0 || padding.left > 0) {
            ss << "padding: " << padding.top << "px " << padding.right << "px " 
               << padding.bottom << "px " << padding.left << "px; ";
        }
        if (margin.top > 0 || margin.right > 0 || margin.bottom > 0 || margin.left > 0) {
            ss << "margin: " << margin.top << "px " << margin.right << "px " 
               << margin.bottom << "px " << margin.left << "px; ";
        }
        if (flex_direction == FlexDirection::Row) ss << "display: flex; flex-direction: row; ";
        else ss << "display: flex; flex-direction: column; ";
        if (gap > 0) ss << "gap: " << gap << "px; ";
        if (!width.empty()) ss << "width: " << width << "; ";
        if (!height.empty()) ss << "height: " << height << "; ";
        if (max_width > 0) ss << "max-width: " << max_width << "px; ";
        if (align_items == AlignItems::Center) ss << "align-items: center; ";
        if (justify_content == JustifyContent::Center) ss << "justify-content: center; ";
        else if (justify_content == JustifyContent::SpaceBetween) ss << "justify-content: space-between; ";
        if (!shadow.color.empty() && (shadow.x != 0 || shadow.y != 0 || shadow.blur != 0)) {
            ss << "box-shadow: " << shadow.x << "px " << shadow.y << "px " << shadow.blur << "px " << shadow.color << "; ";
        }
        return ss.str();
    }
};

// Simple utility-class parser (Method B: Tailwind-compatible engine)
inline void apply_utility_class(ComputedStyle& s, const std::string& token) {
    if (token == "bg-white") s.background_color = "#FFFFFF";
    else if (token == "bg-blue-600") s.background_color = "#2563EB";
    else if (token == "bg-slate-100") s.background_color = "#F1F5F9";
    else if (token == "bg-slate-800") s.background_color = "#1E293B";
    else if (token == "text-white") s.text_color = "#FFFFFF";
    else if (token == "text-blue-600") s.text_color = "#2563EB";
    else if (token == "text-slate-800") s.text_color = "#1E293B";
    else if (token == "text-slate-500") s.text_color = "#64748B";
    else if (token == "p-3") s.padding = Insets(12);
    else if (token == "p-4") s.padding = Insets(16);
    else if (token == "p-5") s.padding = Insets(20);
    else if (token == "p-6") s.padding = Insets(24);
    else if (token == "px-4") s.padding.left = s.padding.right = 16;
    else if (token == "py-2") s.padding.top = s.padding.bottom = 8;
    else if (token == "py-3") s.padding.top = s.padding.bottom = 12;
    else if (token == "rounded-lg") s.border_radius = 8;
    else if (token == "rounded-xl") s.border_radius = 12;
    else if (token == "rounded-2xl") s.border_radius = 16;
    else if (token == "rounded-full") s.border_radius = 9999;
    else if (token == "gap-2") s.gap = 8;
    else if (token == "gap-3") s.gap = 12;
    else if (token == "gap-4") s.gap = 16;
    else if (token == "flex-row") s.flex_direction = FlexDirection::Row;
    else if (token == "flex-col") s.flex_direction = FlexDirection::Column;
    else if (token == "items-center") s.align_items = AlignItems::Center;
    else if (token == "justify-between") s.justify_content = JustifyContent::SpaceBetween;
    else if (token == "justify-center") s.justify_content = JustifyContent::Center;
    else if (token == "w-full") s.width = "100%";
    else if (token == "font-bold") s.font_weight = FontWeight::Bold;
    else if (token == "font-medium") s.font_weight = FontWeight::Medium;
    else if (token == "text-xl") s.font_size = 20;
    else if (token == "text-2xl") s.font_size = 24;
    else if (token == "text-sm") s.font_size = 14;
    else if (token == "shadow-sm") s.shadow = BoxShadow{0, 1, 2, "#0000000D"};
    else if (token == "shadow-md") s.shadow = BoxShadow{0, 4, 6, "#0000001A"};
}

inline void apply_utility_classes(ComputedStyle& s, const std::string& class_names) {
    std::istringstream iss(class_names);
    std::string token;
    while (iss >> token) {
        s.class_names.push_back(token);
        apply_utility_class(s, token);
    }
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

    Derived& border(float width, const std::string& color) {
        style_.border_width = width;
        style_.border_color = color;
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

    Derived& shadow(float x, float y, float blur, const std::string& col) {
        style_.shadow = BoxShadow{x, y, blur, col};
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

    Derived& variant(ButtonVariant v) {
        style_.variant = v;
        if (v == ButtonVariant::Primary) {
            style_.background_color = "#2563EB";
            style_.text_color = "#FFFFFF";
        } else if (v == ButtonVariant::Secondary) {
            style_.background_color = "#F1F5F9";
            style_.text_color = "#1E293B";
        } else if (v == ButtonVariant::Destructive) {
            style_.background_color = "#DC2626";
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

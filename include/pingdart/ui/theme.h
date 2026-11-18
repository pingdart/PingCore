// include/pingdart/ui/theme.h
// PingDartUI Configurable Design Tokens & Bundled Palettes (PKG-136)
#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <memory>
#include <cstdint>

namespace pingdart::ui {

struct ColorPalette {
    std::string s50;
    std::string s100;
    std::string s200;
    std::string s300;
    std::string s400;
    std::string s500;
    std::string s600;
    std::string s700;
    std::string s800;
    std::string s900;
    std::string s950;

    const std::string& get(int shade) const {
        switch (shade) {
            case 50: return s50;
            case 100: return s100;
            case 200: return s200;
            case 300: return s300;
            case 400: return s400;
            case 500: return s500;
            case 600: return s600;
            case 700: return s700;
            case 800: return s800;
            case 900: return s900;
            case 950: return s950;
            default: return s500;
        }
    }
};

struct ThemeTokens {
    std::string primary{"#2563EB"};       // Blue 600
    std::string primary_hover{"#1D4ED8"}; // Blue 700
    std::string secondary{"#64748B"};     // Slate 500
    std::string success{"#16A34A"};       // Emerald 600
    std::string danger{"#DC2626"};        // Red 600
    std::string warning{"#D97706"};       // Amber 600
    std::string background{"#FFFFFF"};
    std::string foreground{"#0F172A"};    // Slate 900
    std::string card{"#FFFFFF"};
    std::string card_foreground{"#0F172A"};
    std::string border{"#E2E8F0"};        // Slate 200
    std::string muted{"#F1F5F9"};         // Slate 100
    std::string muted_foreground{"#64748B"};
    float border_radius{12.0f};
    float spacing_unit{4.0f};
    std::string font_family{"Inter, system-ui, -apple-system, sans-serif"};
    std::string dark_mode{"system"};      // "system", "light", "dark"
};

class ThemeEngine {
public:
    static ThemeEngine& instance() {
        static ThemeEngine s_instance;
        return s_instance;
    }

    ThemeEngine() {
        init_bundled_palettes();
        set_light_defaults();
    }

    void set_dark_mode(bool is_dark) {
        is_dark_active_ = is_dark;
        if (is_dark) {
            tokens_.background = "#0F172A";       // Slate 900
            tokens_.foreground = "#F8FAFC";       // Slate 50
            tokens_.card = "#1E293B";             // Slate 800
            tokens_.card_foreground = "#F8FAFC";
            tokens_.border = "#334155";           // Slate 700
            tokens_.muted = "#1E293B";
            tokens_.muted_foreground = "#94A3B8"; // Slate 400
        } else {
            set_light_defaults();
        }
    }

    bool is_dark() const {
        return is_dark_active_;
    }

    bool is_dark_mode() const {
        return is_dark_active_;
    }

    const ThemeTokens& tokens() const {
        return tokens_;
    }

    ThemeTokens& tokens() {
        return tokens_;
    }

    void set_variable(const std::string& name, const std::string& val) {
        variables_[name] = val;
    }

    std::string get_variable(const std::string& name, const std::string& fallback = "") const {
        auto it = variables_.find(name);
        if (it != variables_.end()) return it->second;

        // Built-in theme token mappings
        if (name == "--primary") return tokens_.primary;
        if (name == "--secondary") return tokens_.secondary;
        if (name == "--success") return tokens_.success;
        if (name == "--danger") return tokens_.danger;
        if (name == "--warning") return tokens_.warning;
        if (name == "--background") return tokens_.background;
        if (name == "--foreground") return tokens_.foreground;
        if (name == "--border") return tokens_.border;
        if (name == "--card") return tokens_.card;
        if (name == "--radius") return std::to_string(static_cast<int>(tokens_.border_radius)) + "px";

        return fallback;
    }

    const ColorPalette* get_palette(const std::string& name) const {
        auto it = palettes_.find(name);
        if (it != palettes_.end()) return &it->second;
        return nullptr;
    }

    std::string get_color(const std::string& palette_name, int shade) const {
        const auto* pal = get_palette(palette_name);
        if (pal) return pal->get(shade);
        return "";
    }

    bool load_config_json(const std::string& json_str) {
        // Minimal deterministic JSON token extractor without external deps
        auto extract_field = [&](const std::string& key) -> std::string {
            std::string pattern = "\"" + key + "\":";
            size_t pos = json_str.find(pattern);
            if (pos == std::string::npos) {
                pattern = "\"" + key + "\" :";
                pos = json_str.find(pattern);
            }
            if (pos == std::string::npos) return "";
            size_t val_start = json_str.find_first_not_of(" \t\n\r:", pos + pattern.size() - 1);
            if (val_start == std::string::npos) return "";

            if (json_str[val_start] == '\"') {
                size_t val_end = json_str.find('\"', val_start + 1);
                if (val_end != std::string::npos) {
                    return json_str.substr(val_start + 1, val_end - val_start - 1);
                }
            } else {
                size_t val_end = json_str.find_first_of(",}\n\r", val_start);
                if (val_end != std::string::npos) {
                    std::string num = json_str.substr(val_start, val_end - val_start);
                    num.erase(std::remove_if(num.begin(), num.end(), ::isspace), num.end());
                    return num;
                }
            }
            return "";
        };

        std::string prim = extract_field("primary");
        if (!prim.empty()) tokens_.primary = prim;

        std::string sec = extract_field("secondary");
        if (!sec.empty()) tokens_.secondary = sec;

        std::string succ = extract_field("success");
        if (!succ.empty()) tokens_.success = succ;

        std::string dang = extract_field("danger");
        if (!dang.empty()) tokens_.danger = dang;

        std::string rad = extract_field("borderRadius");
        if (!rad.empty()) {
            try { tokens_.border_radius = std::stof(rad); } catch (...) {}
        }

        std::string sp = extract_field("spacingUnit");
        if (!sp.empty()) {
            try { tokens_.spacing_unit = std::stof(sp); } catch (...) {}
        }

        std::string ff = extract_field("fontFamily");
        if (!ff.empty()) tokens_.font_family = ff;

        std::string dm = extract_field("darkMode");
        if (!dm.empty()) tokens_.dark_mode = dm;

        return true;
    }

private:
    void set_light_defaults() {
        tokens_.background = "#FFFFFF";
        tokens_.foreground = "#0F172A";
        tokens_.card = "#FFFFFF";
        tokens_.card_foreground = "#0F172A";
        tokens_.border = "#E2E8F0";
        tokens_.muted = "#F1F5F9";
        tokens_.muted_foreground = "#64748B";
    }

    void init_bundled_palettes() {
        palettes_["slate"] = {
            "#F8FAFC", "#F1F5F9", "#E2E8F0", "#CBD5E1", "#94A3B8",
            "#64748B", "#475569", "#334155", "#1E293B", "#0F172A", "#020617"
        };
        palettes_["zinc"] = {
            "#FAFAFA", "#F4F4F5", "#E4E4E7", "#D4D4D8", "#A1A1AA",
            "#71717A", "#52525B", "#3F3F46", "#27272A", "#18181B", "#09090B"
        };
        palettes_["red"] = {
            "#FEF2F2", "#FEE2E2", "#FECACA", "#FCA5A5", "#F87171",
            "#EF4444", "#DC2626", "#B91C1C", "#991B1B", "#7F1D1D", "#450A0A"
        };
        palettes_["amber"] = {
            "#FFFBEB", "#FEF3C7", "#FDE68A", "#FCD34D", "#FBBF24",
            "#F59E0B", "#D97706", "#B45309", "#92400E", "#78350F", "#451A03"
        };
        palettes_["emerald"] = {
            "#ECFDF5", "#D1FAE5", "#A7F3D0", "#6EE7B7", "#34D399",
            "#10B981", "#059669", "#047857", "#065F46", "#064E3B", "#022C22"
        };
        palettes_["blue"] = {
            "#EFF6FF", "#DBEAFE", "#BFDBFE", "#93C5FD", "#60A5FA",
            "#3B82F6", "#2563EB", "#1D4ED8", "#1E40AF", "#1E3A8A", "#172554"
        };
        palettes_["indigo"] = {
            "#EEF2FF", "#E0E7FF", "#C7D2FE", "#A5B4FC", "#818CF8",
            "#6366F1", "#4F46E5", "#4338CA", "#3730A3", "#312E81", "#1E1B4B"
        };
        palettes_["violet"] = {
            "#F5F3FF", "#EDE9FE", "#DDD6FE", "#C4B5FD", "#A78BFA",
            "#8B5CF6", "#7C3AED", "#6D28D9", "#5B21B6", "#4C1D95", "#2E1065"
        };
    }

    ThemeTokens tokens_{};
    bool is_dark_active_{false};
    std::unordered_map<std::string, ColorPalette> palettes_{};
    std::unordered_map<std::string, std::string> variables_{};
};

} // namespace pingdart::ui

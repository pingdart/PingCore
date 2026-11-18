// include/pingdart/ui/css_parser.h
// PingDartUI CSS Parser, Specificity Engine, & HTML Sanitizer (PKG-136)
#pragma once

#include <pingdart/ui/theme.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <tuple>
#include <optional>

namespace pingdart::ui {

// 1. Security: HTML, URL, QML, & CSS Sanitizers (XSS Prevention & Code Injection Defense)
inline std::string escape_html(std::string_view input) {
    std::string out;
    out.reserve(input.size() * 11 / 10);
    for (char c : input) {
        switch (c) {
            case '&':  out.append("&amp;"); break;
            case '<':  out.append("&lt;"); break;
            case '>':  out.append("&gt;"); break;
            case '\"': out.append("&quot;"); break;
            case '\'': out.append("&#39;"); break;
            default:   out.push_back(c); break;
        }
    }
    return out;
}

inline std::string sanitize_url(std::string_view url) {
    std::string s(url);
    // Trim leading whitespace
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) s = s.substr(start);

    // Convert to lowercase copy for scheme validation
    std::string lower_s = s;
    std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    // Block dangerous execution schemes
    if (lower_s.starts_with("javascript:") ||
        lower_s.starts_with("vbscript:") ||
        lower_s.starts_with("data:") ||
        lower_s.starts_with("file:")) {
        return "#blocked-unsafe-url";
    }
    return s;
}

inline std::string escape_qml_string(std::string_view input) {
    std::string out;
    out.reserve(input.size() + 16);
    for (char c : input) {
        switch (c) {
            case '\\': out.append("\\\\"); break;
            case '\"': out.append("\\\""); break;
            case '\n': out.append("\\n"); break;
            case '\r': out.append("\\r"); break;
            case '\t': out.append("\\t"); break;
            default:   out.push_back(c); break;
        }
    }
    return out;
}

inline std::string sanitize_css_value(std::string_view val) {
    std::string s(val);
    std::string lower_s = s;
    std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (lower_s.find("javascript:") != std::string::npos ||
        lower_s.find("expression(") != std::string::npos ||
        lower_s.find("behavior:") != std::string::npos) {
        return "inherit";
    }
    return s;
}

// 2. Specificity Calculation: Lexicographical 4-tuple (a=inline, b=ID, c=class/pseudo, d=tag)
struct CssSpecificity {
    uint32_t a{0}; // Inline style
    uint32_t b{0}; // ID count
    uint32_t c{0}; // Class / pseudo-class count
    uint32_t d{0}; // Tag / element count

    // C++20 default <=> performs strict lexicographical member-wise comparison in order (a, b, c, d)
    auto operator<=>(const CssSpecificity&) const = default;
    bool operator==(const CssSpecificity&) const = default;

    // Optional legacy scalar score for debug logging only
    uint64_t score() const {
        return (static_cast<uint64_t>(a) * 1000000000ULL) +
               (static_cast<uint64_t>(b) * 1000000ULL) +
               (static_cast<uint64_t>(c) * 1000ULL) +
               static_cast<uint64_t>(d);
    }
};

struct CssDeclaration {
    std::string property{""};
    std::string value{""};
    bool is_important{false};
};

struct CssSelector {
    std::string tag{""};
    std::string id{""};
    std::vector<std::string> classes{};
    std::string pseudo{""}; // "hover", "focus", "active", "disabled"

    CssSpecificity specificity() const {
        CssSpecificity spec{0, 0, 0, 0};
        if (!id.empty()) spec.b += 1;
        spec.c += static_cast<uint32_t>(classes.size());
        if (!pseudo.empty()) spec.c += 1;
        if (!tag.empty() && tag != "*") spec.d += 1;
        return spec;
    }

    bool matches(const std::string& comp_tag, 
                 const std::vector<std::string>& comp_classes, 
                 const std::string& comp_id,
                 const std::string& comp_pseudo = "") const {
        if (!tag.empty() && tag != "*" && tag != comp_tag) return false;
        if (!id.empty() && id != comp_id) return false;
        if (!pseudo.empty() && pseudo != comp_pseudo) return false;

        for (const auto& required_cls : classes) {
            bool found = false;
            for (const auto& existing_cls : comp_classes) {
                if (existing_cls == required_cls) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }
};

struct CssRule {
    std::vector<CssSelector> selectors{};
    std::vector<CssDeclaration> declarations{};
    CssSpecificity specificity{};
    size_t order{0};
};

class CssParser {
public:
    static std::string trim(const std::string& s) {
        auto start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        auto end = s.find_last_not_of(" \t\n\r");
        return s.substr(start, end - start + 1);
    }

    static std::string resolve_variables(const std::string& val, const ThemeEngine& theme) {
        std::string res = val;
        size_t pos = 0;
        while ((pos = res.find("var(", pos)) != std::string::npos) {
            size_t close = res.find(')', pos);
            if (close == std::string::npos) break;

            std::string content = res.substr(pos + 4, close - pos - 4);
            std::string var_name = content;
            std::string fallback = "";

            size_t comma = content.find(',');
            if (comma != std::string::npos) {
                var_name = trim(content.substr(0, comma));
                fallback = trim(content.substr(comma + 1));
            } else {
                var_name = trim(content);
            }

            std::string resolved = theme.get_variable(var_name, fallback);
            if (resolved.empty()) resolved = fallback;

            res.replace(pos, close - pos + 1, resolved);
            pos += resolved.size();
        }
        return res;
    }

    static float parse_px_dimension(const std::string& val, float base_font_size = 16.0f) {
        std::string s = trim(val);
        if (s.empty()) return 0.0f;

        try {
            if (s.ends_with("px")) {
                return std::stof(s.substr(0, s.size() - 2));
            } else if (s.ends_with("rem")) {
                return std::stof(s.substr(0, s.size() - 3)) * base_font_size;
            } else if (s.ends_with("em")) {
                return std::stof(s.substr(0, s.size() - 2)) * base_font_size;
            } else if (s.ends_with("%")) {
                return std::stof(s.substr(0, s.size() - 1));
            } else {
                return std::stof(s);
            }
        } catch (...) {
            return 0.0f;
        }
    }

    static std::vector<CssRule> parse(const std::string& css_text, std::vector<std::string>* out_diagnostics = nullptr) {
        std::vector<CssRule> rules;
        size_t pos = 0;
        size_t len = css_text.size();
        size_t rule_order = 0;

        while (pos < len) {
            // Skip comments and whitespace
            pos = skip_whitespace_and_comments(css_text, pos);
            if (pos >= len) break;

            size_t brace_open = css_text.find('{', pos);
            if (brace_open == std::string::npos) {
                if (out_diagnostics && pos < len) {
                    out_diagnostics->push_back("Syntax warning: Trailing tokens without declaration block.");
                }
                break;
            }

            std::string selector_str = trim(css_text.substr(pos, brace_open - pos));
            size_t brace_close = css_text.find('}', brace_open);
            if (brace_close == std::string::npos) {
                if (out_diagnostics) {
                    out_diagnostics->push_back("Syntax error: Unclosed declaration block at position " + std::to_string(brace_open));
                }
                break;
            }

            std::string body_str = css_text.substr(brace_open + 1, brace_close - brace_open - 1);
            pos = brace_close + 1;

            if (selector_str.empty()) continue;

            // Parse selectors (comma separated)
            auto selectors = parse_selectors(selector_str);
            auto declarations = parse_declarations(body_str);

            for (const auto& sel : selectors) {
                CssRule rule;
                rule.selectors.push_back(sel);
                rule.declarations = declarations;
                rule.specificity = sel.specificity();
                rule.order = rule_order++;
                rules.push_back(rule);
            }
        }

        return rules;
    }

private:
    static size_t skip_whitespace_and_comments(const std::string& s, size_t pos) {
        while (pos < s.size()) {
            if (std::isspace(static_cast<unsigned char>(s[pos]))) {
                pos++;
            } else if (pos + 1 < s.size() && s[pos] == '/' && s[pos + 1] == '*') {
                size_t end = s.find("*/", pos + 2);
                if (end == std::string::npos) return s.size();
                pos = end + 2;
            } else {
                break;
            }
        }
        return pos;
    }

    static std::vector<CssSelector> parse_selectors(const std::string& selector_text) {
        std::vector<CssSelector> res;
        std::istringstream iss(selector_text);
        std::string token;

        while (std::getline(iss, token, ',')) {
            token = trim(token);
            if (token.empty()) continue;

            CssSelector sel;
            // Parse pseudo-class
            size_t colon = token.find(':');
            if (colon != std::string::npos) {
                sel.pseudo = token.substr(colon + 1);
                token = token.substr(0, colon);
            }

            // Parse ID
            size_t hash = token.find('#');
            if (hash != std::string::npos) {
                sel.id = token.substr(hash + 1);
                token = token.substr(0, hash);
            }

            // Parse classes
            size_t dot_pos = token.find('.');
            if (dot_pos != std::string::npos) {
                sel.tag = trim(token.substr(0, dot_pos));
                std::string class_part = token.substr(dot_pos);
                std::istringstream c_iss(class_part);
                std::string cls;
                while (std::getline(c_iss, cls, '.')) {
                    cls = trim(cls);
                    if (!cls.empty()) sel.classes.push_back(cls);
                }
            } else {
                sel.tag = trim(token);
            }

            res.push_back(sel);
        }
        return res;
    }

    static std::vector<CssDeclaration> parse_declarations(const std::string& body_text) {
        std::vector<CssDeclaration> decls;
        std::istringstream iss(body_text);
        std::string statement;

        while (std::getline(iss, statement, ';')) {
            statement = trim(statement);
            if (statement.empty()) continue;

            size_t colon = statement.find(':');
            if (colon == std::string::npos) continue;

            std::string prop = trim(statement.substr(0, colon));
            std::string val = trim(statement.substr(colon + 1));

            bool important = false;
            size_t imp_pos = val.find("!important");
            if (imp_pos != std::string::npos) {
                important = true;
                val = trim(val.substr(0, imp_pos));
            }

            decls.push_back(CssDeclaration{prop, val, important});
        }
        return decls;
    }
};

} // namespace pingdart::ui

#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace pingdart::web {

class UrlSecurity {
public:
    // Decodes %XX hex sequences. Returns false if any sequence is malformed or invalid.
    [[nodiscard]] static bool percent_decode(std::string_view encoded, std::string& out) {
        out.clear();
        out.reserve(encoded.size());

        for (size_t i = 0; i < encoded.size(); ++i) {
            char c = encoded[i];
            if (c == '%') {
                if (i + 2 >= encoded.size()) {
                    return false; // Truncated escape
                }
                char h1 = encoded[i + 1];
                char h2 = encoded[i + 2];
                int v1 = hex_digit(h1);
                int v2 = hex_digit(h2);
                if (v1 < 0 || v2 < 0) {
                    return false; // Invalid hex
                }
                char decoded_char = static_cast<char>((v1 << 4) | v2);
                out.push_back(decoded_char);
                i += 2;
            } else {
                out.push_back(c);
            }
        }
        return true;
    }

    // Validates that a route parameter (e.g. {slug}) is clean, bounded, and cannot escape directories
    [[nodiscard]] static bool is_safe_route_parameter(std::string_view param) noexcept {
        if (param.empty() || param.size() > 128) {
            return false;
        }
        if (param == "." || param == "..") {
            return false;
        }
        for (char c : param) {
            const bool is_alphanumeric = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
            const bool is_safe_punct = (c == '-' || c == '_' || c == '.');
            if (!is_alphanumeric && !is_safe_punct) {
                return false;
            }
        }
        return true;
    }

    // Validates full URI path: percent decodes, checks for directory traversal, null bytes, and script injection
    [[nodiscard]] static bool is_safe_uri_path(std::string_view raw_path) {
        if (raw_path.empty() || raw_path.size() > 2048) {
            return false;
        }

        std::string decoded;
        if (!percent_decode(raw_path, decoded)) {
            return false;
        }

        // Null bytes or control characters
        for (char c : decoded) {
            auto uc = static_cast<unsigned char>(c);
            if (uc < 32 || uc == 127 || uc == '<' || uc == '>') {
                return false;
            }
        }

        // Directory traversal checks on decoded string
        if (decoded.find("..") != std::string::npos) {
            return false;
        }

        return true;
    }

private:
    [[nodiscard]] static constexpr int hex_digit(char c) noexcept {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }
};

} // namespace pingdart::web

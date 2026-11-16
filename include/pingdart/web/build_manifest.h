#pragma once

#include <string>
#include <vector>

namespace pingdart::web {

struct AssetEntry {
    std::string original_name;
    std::string hashed_name;
    std::string hash;
    size_t byte_size{0};
};

struct BuildManifest {
    std::string pingdart_version{"0.1.0"};
    std::string build_timestamp;
    std::vector<std::string> routes;
    std::vector<AssetEntry> assets;
    std::string sitemap_path{"sitemap.xml"};
    std::string robots_path{"robots.txt"};

    [[nodiscard]] std::string to_json() const {
        std::string json;
        json.reserve(2048);
        json.append("{\n");
        json.append("  \"pingdart_version\": \"");
        json.append(pingdart_version);
        json.append("\",\n");

        if (!build_timestamp.empty()) {
            json.append("  \"build_timestamp\": \"");
            json.append(build_timestamp);
            json.append("\",\n");
        }

        // Routes array
        json.append("  \"routes\": [\n");
        for (size_t i = 0; i < routes.size(); ++i) {
            json.append("    \"");
            json.append(routes[i]);
            json.append("\"");
            if (i + 1 < routes.size()) json.append(",");
            json.append("\n");
        }
        json.append("  ],\n");

        // Assets array
        json.append("  \"assets\": [\n");
        for (size_t i = 0; i < assets.size(); ++i) {
            const auto& a = assets[i];
            json.append("    {\n");
            json.append("      \"original\": \"");
            json.append(a.original_name);
            json.append("\",\n");
            json.append("      \"hashed\": \"");
            json.append(a.hashed_name);
            json.append("\",\n");
            json.append("      \"hash\": \"");
            json.append(a.hash);
            json.append("\",\n");
            json.append("      \"size_bytes\": ");
            json.append(std::to_string(a.byte_size));
            json.append("\n    }");
            if (i + 1 < assets.size()) json.append(",");
            json.append("\n");
        }
        json.append("  ],\n");

        json.append("  \"sitemap\": \"");
        json.append(sitemap_path);
        json.append("\",\n");

        json.append("  \"robots\": \"");
        json.append(robots_path);
        json.append("\"\n");

        json.append("}\n");
        return json;
    }
};

} // namespace pingdart::web

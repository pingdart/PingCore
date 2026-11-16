#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace pingdart::web {

struct OpenGraphMetadata {
    std::string title;
    std::string description;
    std::string url;
    std::string image_url;
    std::string type{"website"};
    std::string site_name;
};

struct TwitterCardMetadata {
    std::string card{"summary_large_image"};
    std::string site;
    std::string creator;
    std::string title;
    std::string description;
    std::string image_url;
};

struct SeoMetadata {
    std::string title;
    std::string description;
    std::string canonical_url;
    std::string robots{"index, follow"};
    std::string locale{"en_US"};
    std::string keywords;

    // hreflang alternate links: pair<locale_code, url> (e.g. {"es", "https://example.com/es/doctors"})
    std::vector<std::pair<std::string, std::string>> alternate_locales;

    // Social Graph
    OpenGraphMetadata open_graph{};
    TwitterCardMetadata twitter_card{};

    // Schema.org JSON-LD structured data payloads
    std::vector<std::string> json_ld_schemas;

    // Escape HTML special characters (&, <, >, ", ')
    [[nodiscard]] static std::string escape_html(std::string_view input) {
        std::string output;
        output.reserve(input.size() + 16);
        for (char c : input) {
            switch (c) {
                case '&':  output.append("&amp;"); break;
                case '<':  output.append("&lt;"); break;
                case '>':  output.append("&gt;"); break;
                case '\"': output.append("&quot;"); break;
                case '\'': output.append("&#39;"); break;
                default:   output.push_back(c); break;
            }
        }
        return output;
    }

    // Sanitizes JSON string so it can be safely embedded inside <script type="application/ld+json">
    // without risking script-tag breakout (</script>), HTML comment escapes (<!--), or Unicode newlines.
    [[nodiscard]] static std::string sanitize_json_for_html_script(std::string_view json) {
        std::string output;
        output.reserve(json.size() + 32);
        for (size_t i = 0; i < json.size(); ++i) {
            char c = json[i];
            switch (c) {
                case '<': output.append("\\u003c"); break;
                case '>': output.append("\\u003e"); break;
                case '&': output.append("\\u0026"); break;
                default:  output.push_back(c); break;
            }
        }
        return output;
    }

    // Renders complete, valid HTML5 <head> tags for crawlers and browsers
    [[nodiscard]] std::string render_head_tags() const {
        std::string html;
        html.reserve(1024);

        if (!title.empty()) {
            html.append("  <title>");
            html.append(escape_html(title));
            html.append("</title>\n");
        }

        if (!description.empty()) {
            html.append("  <meta name=\"description\" content=\"");
            html.append(escape_html(description));
            html.append("\">\n");
        }

        if (!keywords.empty()) {
            html.append("  <meta name=\"keywords\" content=\"");
            html.append(escape_html(keywords));
            html.append("\">\n");
        }

        if (!robots.empty()) {
            html.append("  <meta name=\"robots\" content=\"");
            html.append(escape_html(robots));
            html.append("\">\n");
        }

        if (!canonical_url.empty()) {
            html.append("  <link rel=\"canonical\" href=\"");
            html.append(escape_html(canonical_url));
            html.append("\">\n");
        }

        for (const auto& [lang, url] : alternate_locales) {
            html.append("  <link rel=\"alternate\" hreflang=\"");
            html.append(escape_html(lang));
            html.append("\" href=\"");
            html.append(escape_html(url));
            html.append("\">\n");
        }

        // OpenGraph
        const std::string og_title = open_graph.title.empty() ? title : open_graph.title;
        const std::string og_desc = open_graph.description.empty() ? description : open_graph.description;
        const std::string og_url = open_graph.url.empty() ? canonical_url : open_graph.url;

        if (!og_title.empty()) {
            html.append("  <meta property=\"og:title\" content=\"");
            html.append(escape_html(og_title));
            html.append("\">\n");
        }
        if (!og_desc.empty()) {
            html.append("  <meta property=\"og:description\" content=\"");
            html.append(escape_html(og_desc));
            html.append("\">\n");
        }
        if (!og_url.empty()) {
            html.append("  <meta property=\"og:url\" content=\"");
            html.append(escape_html(og_url));
            html.append("\">\n");
        }
        if (!open_graph.image_url.empty()) {
            html.append("  <meta property=\"og:image\" content=\"");
            html.append(escape_html(open_graph.image_url));
            html.append("\">\n");
        }
        if (!open_graph.type.empty()) {
            html.append("  <meta property=\"og:type\" content=\"");
            html.append(escape_html(open_graph.type));
            html.append("\">\n");
        }
        if (!open_graph.site_name.empty()) {
            html.append("  <meta property=\"og:site_name\" content=\"");
            html.append(escape_html(open_graph.site_name));
            html.append("\">\n");
        }

        // Twitter Card
        if (!twitter_card.card.empty()) {
            html.append("  <meta name=\"twitter:card\" content=\"");
            html.append(escape_html(twitter_card.card));
            html.append("\">\n");
        }
        if (!twitter_card.title.empty() || !title.empty()) {
            html.append("  <meta name=\"twitter:title\" content=\"");
            html.append(escape_html(twitter_card.title.empty() ? title : twitter_card.title));
            html.append("\">\n");
        }
        if (!twitter_card.description.empty() || !description.empty()) {
            html.append("  <meta name=\"twitter:description\" content=\"");
            html.append(escape_html(twitter_card.description.empty() ? description : twitter_card.description));
            html.append("\">\n");
        }
        if (!twitter_card.image_url.empty() || !open_graph.image_url.empty()) {
            html.append("  <meta name=\"twitter:image\" content=\"");
            html.append(escape_html(twitter_card.image_url.empty() ? open_graph.image_url : twitter_card.image_url));
            html.append("\">\n");
        }
        if (!twitter_card.site.empty()) {
            html.append("  <meta name=\"twitter:site\" content=\"");
            html.append(escape_html(twitter_card.site));
            html.append("\">\n");
        }

        // Structured Data JSON-LD (HTML-safe embedded)
        for (const auto& schema_json : json_ld_schemas) {
            html.append("  <script type=\"application/ld+json\">\n");
            html.append(sanitize_json_for_html_script(schema_json));
            html.append("\n  </script>\n");
        }

        return html;
    }
};

} // namespace pingdart::web

// include/pingcore/web_ssr.h
// Server-Side HTML Rendering (SSR) & Static Site Generation (SSG) for PingCore (PKG-120)
#pragma once

#include <pingcore/ui.h>
#include <pingcore/router.h>
#include <pingdart/web/seo_metadata.h>
#include <pingdart/web/url_security.h>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace pingcore {

struct SsrRenderOptions {
    std::string title{"PingCore Application"};
    std::string description{"High-performance cross-platform application built with PingCore C++20"};
    std::string canonical_url{"https://example.com"};
    std::string language{"en"};
    bool include_seo_meta{true};
    bool pretty_print{true};
};

class SsrHtmlRenderer {
public:
    static std::string render_document(const Widget& root_widget, const SsrRenderOptions& options = {}) {
        std::ostringstream ss;
        ss << "<!doctype html>\n";
        ss << "<html lang=\"" << options.language << "\">\n";
        ss << "<head>\n";
        ss << "  <meta charset=\"utf-8\" />\n";
        ss << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n";
        ss << "  <title>" << escape_html(options.title) << "</title>\n";

        if (options.include_seo_meta) {
            ss << "  <meta name=\"description\" content=\"" << escape_html(options.description) << "\" />\n";
            ss << "  <meta property=\"og:title\" content=\"" << escape_html(options.title) << "\" />\n";
            ss << "  <meta property=\"og:description\" content=\"" << escape_html(options.description) << "\" />\n";
            ss << "  <meta property=\"og:type\" content=\"website\" />\n";
            ss << "  <meta property=\"og:url\" content=\"" << escape_html(options.canonical_url) << "\" />\n";
            ss << "  <link rel=\"canonical\" href=\"" << escape_html(options.canonical_url) << "\" />\n";
        }

        // Embedded base CSS for PingCore layout fidelity
        ss << "  <style>\n"
           << "    * { box-sizing: border-box; margin: 0; padding: 0; }\n"
           << "    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background-color: #0f172a; color: #f8fafc; min-height: 100vh; }\n"
           << "    .pingcore-center { display: flex; justify-content: center; align-items: center; min-height: 100vh; width: 100%; }\n"
           << "    .pingcore-column { display: flex; flex-direction: column; align-items: stretch; width: 100%; max-width: 480px; }\n"
           << "    .pingcore-row { display: flex; flex-direction: row; align-items: center; width: 100%; }\n"
           << "    .pingcore-text { margin-bottom: 0.5rem; line-height: 1.5; }\n"
           << "    .pingcore-btn { display: inline-block; padding: 0.75rem 1.5rem; border-radius: 0.5rem; background-color: #3b82f6; color: #ffffff; border: none; font-weight: 600; cursor: pointer; text-align: center; transition: background-color 0.2s; }\n"
           << "    .pingcore-btn:hover { background-color: #2563eb; }\n"
           << "    .pingcore-input { width: 100%; padding: 0.75rem 1rem; border-radius: 0.5rem; background-color: #1e293b; border: 1px solid #334155; color: #f8fafc; font-size: 1rem; margin-bottom: 0.75rem; }\n"
           << "    .pingcore-input:focus { outline: 2px solid #3b82f6; }\n"
           << "  </style>\n";

        ss << "</head>\n";
        ss << "<body>\n";
        ss << "  <div id=\"__pingcore_root\" data-hydrated=\"false\">\n";
        ss << render_widget(root_widget, 2);
        ss << "  </div>\n";
        ss << "  <script src=\"/_pingcore/client.js\" defer></script>\n";
        ss << "</body>\n";
        ss << "</html>\n";
        return ss.str();
    }

    static std::string render_widget(const Widget& widget, int indent_level = 0) {
        if (!widget) return "";
        std::string indent(static_cast<size_t>(indent_level * 2), ' ');
        std::string type = std::string(widget->widget_type());

        if (type == "Center") {
            std::ostringstream ss;
            ss << indent << "<div class=\"pingcore-center\">\n";
            // Check if widget has children via element construction or introspection
            ss << indent << "</div>\n";
            return ss.str();
        } else if (type == "Text") {
            return indent + "<p class=\"pingcore-text\">" + escape_html(type) + "</p>\n";
        } else if (type == "Button") {
            return indent + "<button class=\"pingcore-btn\" type=\"button\">Action</button>\n";
        } else if (type == "TextField") {
            return indent + "<input class=\"pingcore-input\" type=\"text\" />\n";
        }

        // Generic container
        return indent + "<div class=\"pingcore-widget\" data-type=\"" + type + "\"></div>\n";
    }

    // Static Site Generation (SSG) for full project
    static bool generate_static_site(Router& r, const std::filesystem::path& output_dir, const std::string& app_name) {
        std::error_code ec;
        std::filesystem::create_directories(output_dir, ec);
        if (ec) return false;

        // 1. Generate HTML for known routes
        std::vector<std::string> routes = {"/", "/login", "/dashboard"};
        for (const auto& path : routes) {
            r.navigate(path);
            Widget page = r.render_current_page();
            SsrRenderOptions opts;
            opts.title = app_name + (path == "/" ? " - Home" : " - " + path.substr(1));
            opts.canonical_url = "https://example.com" + path;
            std::string html = render_document(page, opts);

            std::filesystem::path file_path = output_dir;
            if (path == "/") {
                file_path /= "index.html";
            } else {
                file_path /= (path.substr(1) + ".html");
            }

            std::ofstream f(file_path);
            if (f.is_open()) {
                f << html;
            }
        }

        // 2. Generate sitemap.xml
        {
            std::ofstream sm(output_dir / "sitemap.xml");
            sm << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
               << "<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n";
            for (const auto& path : routes) {
                sm << "  <url>\n"
                   << "    <loc>https://example.com" << path << "</loc>\n"
                   << "    <changefreq>daily</changefreq>\n"
                   << "    <priority>" << (path == "/" ? "1.0" : "0.8") << "</priority>\n"
                   << "  </url>\n";
            }
            sm << "</urlset>\n";
        }

        // 3. Generate robots.txt
        {
            std::ofstream rb(output_dir / "robots.txt");
            rb << "User-agent: *\n"
               << "Allow: /\n"
               << "Sitemap: https://example.com/sitemap.xml\n";
        }

        return true;
    }

    static std::string escape_html(std::string_view text) {
        std::string out;
        out.reserve(text.size() + 8);
        for (char c : text) {
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
};

} // namespace pingcore

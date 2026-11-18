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
#include <algorithm>
#include <cmath>
#include <iomanip>

namespace pingcore {

struct SsrRenderOptions {
    std::string title{"PingCore Application"};
    std::string description{"High-performance cross-platform application built with PingCore C++20"};
    std::string canonical_url{"https://example.com"};
    std::string language{"en"};
    bool include_seo_meta{true};
    bool include_hydration_script{true};
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
           << "    .pingcore-btn { display: inline-block; padding: 0.75rem 1.5rem; border-radius: 0.5rem; background-color: #3b82f6; color: #ffffff; border: none; font-weight: 600; cursor: pointer; text-align: center; text-decoration: none; transition: background-color 0.2s; }\n"
           << "    .pingcore-btn:hover { background-color: #2563eb; }\n"
           << "    .pingcore-input { width: 100%; padding: 0.75rem 1rem; border-radius: 0.5rem; background-color: #1e293b; border: 1px solid #334155; color: #f8fafc; font-size: 1rem; margin-bottom: 0.75rem; }\n"
           << "    .pingcore-input:focus { outline: 2px solid #3b82f6; }\n"
           << "  </style>\n";

        ss << "</head>\n";
        ss << "<body>\n";
        ss << "  <div id=\"__pingcore_root\" data-hydrated=\"false\">\n";
        ss << render_widget(root_widget, 2);
        ss << "  </div>\n";
        if (options.include_hydration_script) {
            ss << "  <script src=\"/_pingcore/client.js\" defer></script>\n";
        }
        ss << "</body>\n";
        ss << "</html>\n";
        return ss.str();
    }

    static std::string render_widget(const Widget& widget, int indent_level = 0) {
        if (!widget) return "";
        std::string indent(static_cast<size_t>(indent_level * 2), ' ');
        const std::string type(widget->widget_type());
        std::ostringstream ss;

        if (const auto text = std::dynamic_pointer_cast<const pingdart::ui::Text>(widget)) {
            ss << indent << "<p class=\"pingcore-text\" style=\"font-size:" << text->font_size()
               << "px;color:" << color_to_css(text->color()) << ';';
            if (text->bold()) ss << "font-weight:700;";
            if (text->italic()) ss << "font-style:italic;";
            ss << "\">" << escape_html(text->text()) << "</p>\n";
        } else if (const auto button = std::dynamic_pointer_cast<const pingdart::ui::Button>(widget)) {
            const auto padding = button->padding();
            const bool is_navigation_link = !button->navigation_target().empty();
            if (is_navigation_link) {
                ss << indent << "<a class=\"pingcore-btn\" href=\""
                   << escape_html(button->navigation_target()) << "\" style=\"background-color:";
            } else {
                ss << indent << "<button class=\"pingcore-btn\" type=\"button\" style=\"background-color:";
            }
            ss << color_to_css(button->background_color()) << ";border-radius:"
               << button->border_radius() << "px;padding:" << padding.top << "px "
               << padding.right << "px " << padding.bottom << "px " << padding.left << "px;\">";
            if (const auto label = std::dynamic_pointer_cast<const pingdart::ui::Text>(button->child())) {
                ss << escape_html(label->text());
            } else {
                ss << render_widget(button->child());
            }
            ss << (is_navigation_link ? "</a>\n" : "</button>\n");
        } else if (const auto field = std::dynamic_pointer_cast<const pingdart::ui::TextField>(widget)) {
            const auto value = field->controller() ? field->controller()->text() : std::string{};
            ss << indent << "<input class=\"pingcore-input\" type=\""
               << (field->obscure_text() ? "password" : "text")
               << "\" placeholder=\"" << escape_html(field->placeholder())
               << "\" value=\"" << escape_html(value) << "\" />\n";
        } else if (const auto container = std::dynamic_pointer_cast<const pingdart::ui::Container>(widget)) {
            ss << indent << "<div class=\"pingcore-container\" style=\"";
            if (std::isfinite(container->width()) && container->width() >= 0.0f) {
                ss << "width:" << container->width() << "px;";
            }
            if (std::isfinite(container->height()) && container->height() >= 0.0f) {
                ss << "height:" << container->height() << "px;";
            }
            if (container->color().a > 0.0f) {
                ss << "background-color:" << color_to_css(container->color()) << ';';
            }
            const auto padding = container->padding();
            if (padding.top != 0.0f || padding.right != 0.0f ||
                padding.bottom != 0.0f || padding.left != 0.0f) {
                ss << "padding:" << padding.top << "px " << padding.right << "px "
                   << padding.bottom << "px " << padding.left << "px;";
            }
            if (container->border_radius() > 0.0f) {
                ss << "border-radius:" << container->border_radius() << "px;";
            }
            ss << "\">\n" << render_widget(container->child(), indent_level + 1)
               << indent << "</div>\n";
        } else if (type == "Center") {
            const auto single_child =
                std::dynamic_pointer_cast<const pingdart::ui::SingleChildRenderObjectWidget>(widget);
            ss << indent << "<div class=\"pingcore-center\">\n"
               << render_widget(single_child ? single_child->child() : nullptr, indent_level + 1)
               << indent << "</div>\n";
        } else if (const auto flex = std::dynamic_pointer_cast<const pingdart::ui::Flex>(widget)) {
            const bool vertical = flex->direction() == pingdart::ui::Axis::Vertical;
            ss << indent << "<div class=\"" << (vertical ? "pingcore-column" : "pingcore-row") << "\">\n";
            for (const auto& child : flex->children()) {
                ss << render_widget(child, indent_level + 1);
            }
            ss << indent << "</div>\n";
        } else if (const auto padding = std::dynamic_pointer_cast<const pingdart::ui::Padding>(widget)) {
            const auto insets = padding->padding();
            ss << indent << "<div style=\"padding:" << insets.top << "px " << insets.right << "px "
               << insets.bottom << "px " << insets.left << "px;\">\n"
               << render_widget(padding->child(), indent_level + 1)
               << indent << "</div>\n";
        } else if (const auto box = std::dynamic_pointer_cast<const pingdart::ui::SizedBox>(widget)) {
            ss << indent << "<div";
            if (std::isfinite(box->width()) && box->width() >= 0.0f) {
                ss << " style=\"width:" << box->width() << "px;";
                if (std::isfinite(box->height()) && box->height() >= 0.0f) {
                    ss << "height:" << box->height() << "px;";
                }
                ss << '"';
            } else if (std::isfinite(box->height()) && box->height() >= 0.0f) {
                ss << " style=\"height:" << box->height() << "px;\"";
            }
            ss << ">" << render_widget(box->child()) << "</div>\n";
        } else if (const auto flexible = std::dynamic_pointer_cast<const pingdart::ui::Flexible>(widget)) {
            return render_widget(flexible->child(), indent_level);
        } else if (const auto single_child =
                       std::dynamic_pointer_cast<const pingdart::ui::SingleChildRenderObjectWidget>(widget)) {
            ss << indent << "<div class=\"pingcore-widget\" data-type=\""
               << escape_html(type) << "\">\n"
               << render_widget(single_child->child(), indent_level + 1)
               << indent << "</div>\n";
        } else if (const auto multi_child =
                       std::dynamic_pointer_cast<const pingdart::ui::MultiChildRenderObjectWidget>(widget)) {
            ss << indent << "<div class=\"pingcore-widget\" data-type=\""
               << escape_html(type) << "\">\n";
            for (const auto& child : multi_child->children()) {
                ss << render_widget(child, indent_level + 1);
            }
            ss << indent << "</div>\n";
        } else {
            ss << indent << "<div class=\"pingcore-widget\" data-type=\""
               << escape_html(type) << "\"></div>\n";
        }
        return ss.str();
    }

    static std::string color_to_css(const pingdart::ui::Color& color) {
        std::ostringstream ss;
        ss << "rgba("
           << static_cast<int>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f) << ","
           << static_cast<int>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f) << ","
           << static_cast<int>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f) << ","
           << std::fixed << std::setprecision(3) << std::clamp(color.a, 0.0f, 1.0f) << ")";
        return ss.str();
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

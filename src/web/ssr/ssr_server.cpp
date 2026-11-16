#include <pingdart/web/ssr_server.h>
#include <pingdart/web/url_security.h>
#include "default_web_renderer.h"

namespace pingdart::web {

SsrServer::SsrServer() : renderer_(std::make_unique<DefaultWebRenderer>()) {}

std::vector<std::string> SsrServer::split_path(std::string_view path) {
    std::vector<std::string> segments;
    size_t start = 0;
    while (start < path.size()) {
        while (start < path.size() && path[start] == '/') {
            start++;
        }
        if (start >= path.size()) break;
        size_t end = path.find('/', start);
        if (end == std::string_view::npos) {
            end = path.size();
        }
        segments.emplace_back(path.substr(start, end - start));
        start = end;
    }
    return segments;
}

bool SsrServer::is_safe_path(std::string_view path) noexcept {
    return UrlSecurity::is_safe_uri_path(path);
}

void SsrServer::get(std::string pattern, SsrRouteHandler handler) {
    auto segments = split_path(pattern);
    routes_.push_back(RouteEntry{
        .pattern = std::move(pattern),
        .segments = std::move(segments),
        .handler = std::move(handler)
    });
}

void SsrServer::configure_seo(
    std::vector<std::pair<std::string, std::string>> routes_and_lastmod,
    bool allow_indexing,
    std::string sitemap_url) {
    // 1. /sitemap.xml
    get("/sitemap.xml", [this, routes = std::move(routes_and_lastmod)](const HttpRequest&, const auto&) {
        std::string xml = renderer_->render_sitemap(routes);
        return HttpResponse::Ok(std::move(xml), "application/xml; charset=utf-8");
    });

    // 2. /robots.txt
    get("/robots.txt", [this, allow = allow_indexing, sitemap = std::move(sitemap_url)](const HttpRequest&, const auto&) {
        std::string txt = renderer_->render_robots_txt(allow, sitemap);
        return HttpResponse::Ok(std::move(txt), "text/plain; charset=utf-8");
    });
}

HttpResponse SsrServer::handle_request(const HttpRequest& req) const {
    if (!is_safe_path(req.path)) {
        return HttpResponse::BadRequest("SecurityViolation: Illegal or path-traversal characters in URI");
    }

    if (req.method != "GET") {
        HttpResponse res;
        res.status_code = 405;
        res.status_text = "Method Not Allowed";
        res.content_type = "text/plain";
        res.body = "Method Not Allowed";
        return res;
    }

    // Extract query string boundary if present
    std::string_view clean_path = req.path;
    size_t qmark = clean_path.find('?');
    if (qmark != std::string_view::npos) {
        clean_path = clean_path.substr(0, qmark);
    }

    auto req_segments = split_path(clean_path);

    for (const auto& entry : routes_) {
        if (entry.segments.size() != req_segments.size()) {
            continue;
        }

        bool match = true;
        std::unordered_map<std::string, std::string> params;

        for (size_t i = 0; i < entry.segments.size(); ++i) {
            const auto& pattern_seg = entry.segments[i];
            const auto& req_seg = req_segments[i];

            if (!pattern_seg.empty() && pattern_seg.front() == '{' && pattern_seg.back() == '}') {
                if (!UrlSecurity::is_safe_route_parameter(req_seg)) {
                    return HttpResponse::BadRequest("SecurityViolation: Invalid or unsafe route parameter");
                }
                std::string param_name = pattern_seg.substr(1, pattern_seg.size() - 2);
                params[param_name] = req_seg;
            } else if (pattern_seg != req_seg) {
                match = false;
                break;
            }
        }

        if (match) {
            return entry.handler(req, params);
        }
    }

    return HttpResponse::NotFound("404 Page Not Found");
}

} // namespace pingdart::web

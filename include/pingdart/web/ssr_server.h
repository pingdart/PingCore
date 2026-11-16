#pragma once

#include "seo_metadata.h"
#include "i_web_renderer.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>

namespace pingdart::web {

struct HttpRequest {
    std::string method{"GET"};
    std::string path{"/"};
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    int status_code{200};
    std::string status_text{"OK"};
    std::string content_type{"text/html; charset=utf-8"};
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    [[nodiscard]] static HttpResponse Ok(std::string body, std::string content_type = "text/html; charset=utf-8") {
        HttpResponse res;
        res.status_code = 200;
        res.status_text = "OK";
        res.content_type = std::move(content_type);
        res.body = std::move(body);
        return res;
    }

    [[nodiscard]] static HttpResponse NotFound(std::string body = "404 Not Found") {
        HttpResponse res;
        res.status_code = 404;
        res.status_text = "Not Found";
        res.content_type = "text/plain; charset=utf-8";
        res.body = std::move(body);
        return res;
    }

    [[nodiscard]] static HttpResponse BadRequest(std::string body = "400 Bad Request") {
        HttpResponse res;
        res.status_code = 400;
        res.status_text = "Bad Request";
        res.content_type = "text/plain; charset=utf-8";
        res.body = std::move(body);
        return res;
    }
};

using SsrRouteHandler = std::function<HttpResponse(
    const HttpRequest& req,
    const std::unordered_map<std::string, std::string>& params
)>;

class SsrServer {
public:
    SsrServer();
    ~SsrServer() = default;

    // Registers a route handler for HTTP GET
    void get(std::string pattern, SsrRouteHandler handler);

    // Configures automatic /sitemap.xml and /robots.txt generation
    void configure_seo(
        std::vector<std::pair<std::string, std::string>> routes_and_lastmod,
        bool allow_indexing = true,
        std::string sitemap_url = "https://example.com/sitemap.xml");

    // Evaluates request and returns formatted HTTP response (thread-safe, reentrant)
    [[nodiscard]] HttpResponse handle_request(const HttpRequest& req) const;

    // Validates that request path does not contain path-traversal attacks (e.g. "../")
    [[nodiscard]] static bool is_safe_path(std::string_view path) noexcept;

private:
    struct RouteEntry {
        std::string pattern;
        std::vector<std::string> segments;
        SsrRouteHandler handler;
    };

    static std::vector<std::string> split_path(std::string_view path);

    std::vector<RouteEntry> routes_;
    std::unique_ptr<IWebRenderer> renderer_;
};

} // namespace pingdart::web

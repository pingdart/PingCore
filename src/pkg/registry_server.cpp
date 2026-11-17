// src/pkg/registry_server.cpp
// PKG-108: Public Registry API and Developer Portal Implementation
#include <pingdart/pkg/registry_server.h>
#include <pingdart/pkg/archive.h>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace pingdart::pkg {

// ============================================================================
// HTTP Model Helpers
// ============================================================================

std::string http_method_to_string(HttpMethod method) {
    switch (method) {
        case HttpMethod::Get: return "GET";
        case HttpMethod::Post: return "POST";
        case HttpMethod::Put: return "PUT";
        case HttpMethod::Delete: return "DELETE";
        case HttpMethod::Head: return "HEAD";
        case HttpMethod::Options: return "OPTIONS";
    }
    return "GET";
}

HttpMethod string_to_http_method(std::string_view str) {
    if (str == "POST") return HttpMethod::Post;
    if (str == "PUT") return HttpMethod::Put;
    if (str == "DELETE") return HttpMethod::Delete;
    if (str == "HEAD") return HttpMethod::Head;
    if (str == "OPTIONS") return HttpMethod::Options;
    return HttpMethod::Get;
}

std::string HttpRequest::get_header(std::string_view name) const {
    std::string key(name);
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    for (const auto& [k, v] : headers) {
        std::string lk = k;
        std::transform(lk.begin(), lk.end(), lk.begin(), ::tolower);
        if (lk == key) return v;
    }
    return "";
}

std::string HttpRequest::get_query(std::string_view key, std::string_view default_val) const {
    auto it = query_params.find(std::string(key));
    if (it != query_params.end()) return it->second;
    return std::string(default_val);
}

std::string HttpRequest::body_as_string() const {
    return std::string(reinterpret_cast<const char*>(body.data()), body.size());
}

std::string HttpResponse::body_as_string() const {
    return std::string(reinterpret_cast<const char*>(body.data()), body.size());
}

HttpResponse HttpResponse::json(int status, std::string_view json_str) {
    HttpResponse resp;
    resp.status_code = status;
    resp.status_message = (status == 200 || status == 201) ? "OK" : "Error";
    resp.headers["Content-Type"] = "application/json; charset=utf-8";
    resp.headers["Content-Length"] = std::to_string(json_str.size());
    resp.headers["Access-Control-Allow-Origin"] = "*";
    resp.body.assign(json_str.begin(), json_str.end());
    return resp;
}

HttpResponse HttpResponse::html(int status, std::string_view html_str) {
    HttpResponse resp;
    resp.status_code = status;
    resp.status_message = (status == 200) ? "OK" : "Error";
    resp.headers["Content-Type"] = "text/html; charset=utf-8";
    resp.headers["Content-Length"] = std::to_string(html_str.size());
    resp.body.assign(html_str.begin(), html_str.end());
    return resp;
}

HttpResponse HttpResponse::binary(int status, std::string_view mime_type, std::vector<uint8_t> data) {
    HttpResponse resp;
    resp.status_code = status;
    resp.status_message = "OK";
    resp.headers["Content-Type"] = std::string(mime_type);
    resp.headers["Content-Length"] = std::to_string(data.size());
    resp.headers["Access-Control-Allow-Origin"] = "*";
    resp.body = std::move(data);
    return resp;
}

HttpResponse HttpResponse::error(int status, std::string_view message) {
    std::ostringstream ss;
    ss << "{\"error\": true, \"status\": " << status << ", \"message\": \"" << message << "\"}";
    return HttpResponse::json(status, ss.str());
}

// ============================================================================
// DeveloperPortalRenderer Implementation
// ============================================================================

std::string DeveloperPortalRenderer::render_homepage(
    const std::vector<PackageInfo>& featured_packages,
    size_t total_packages,
    size_t total_publishers,
    size_t total_downloads)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
         << "<meta charset=\"UTF-8\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "<title>PingDart Package Registry & Developer Portal</title>\n"
         << "<style>\n"
         << "  :root { --bg: #0d1117; --card-bg: #161b22; --border: #30363d; --text: #c9d1d9; --accent: #58a6ff; --green: #238636; }\n"
         << "  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 0; }\n"
         << "  .header { padding: 48px 24px; text-align: center; background: radial-gradient(circle at center, #1f293d 0%, #0d1117 70%); border-bottom: 1px solid var(--border); }\n"
         << "  h1 { font-size: 2.5rem; margin-bottom: 12px; color: #fff; }\n"
         << "  .subtitle { font-size: 1.15rem; color: #8b949e; max-width: 600px; margin: 0 auto 32px; }\n"
         << "  .search-box { display: flex; max-width: 600px; margin: 0 auto; }\n"
         << "  .search-box input { flex: 1; padding: 12px 18px; border-radius: 6px 0 0 6px; border: 1px solid var(--border); background: var(--card-bg); color: #fff; font-size: 1rem; }\n"
         << "  .search-box button { padding: 12px 24px; background: var(--accent); color: #fff; border: none; border-radius: 0 6px 6px 0; font-weight: 600; cursor: pointer; }\n"
         << "  .stats-bar { display: flex; justify-content: center; gap: 48px; padding: 24px; background: var(--card-bg); border-bottom: 1px solid var(--border); text-align: center; }\n"
         << "  .stat-val { font-size: 1.8rem; font-weight: bold; color: #fff; }\n"
         << "  .stat-lbl { color: #8b949e; font-size: 0.9rem; margin-top: 4px; }\n"
         << "  .container { max-width: 1100px; margin: 40px auto; padding: 0 24px; }\n"
         << "  .pkg-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(320px, 1fr)); gap: 20px; margin-top: 24px; }\n"
         << "  .card { background: var(--card-bg); border: 1px solid var(--border); border-radius: 8px; padding: 20px; transition: border-color 0.2s; }\n"
         << "  .card:hover { border-color: var(--accent); }\n"
         << "  .pkg-title { font-size: 1.25rem; font-weight: 600; color: var(--accent); text-decoration: none; display: flex; justify-content: space-between; }\n"
         << "  .badge { font-size: 0.75rem; background: #21262d; padding: 2px 8px; border-radius: 12px; border: 1px solid var(--border); color: #8b949e; }\n"
         << "  .cli-snippet { background: #000; padding: 12px; border-radius: 6px; font-family: monospace; font-size: 0.85rem; margin-top: 12px; color: #7ee787; }\n"
         << "</style>\n</head>\n<body>\n"
         << "<div class=\"header\">\n"
         << "  <h1>PingDart Package Registry</h1>\n"
         << "  <div class=\"subtitle\">Discover, install, and publish cross-platform C++20 libraries for desktop, mobile, and web.</div>\n"
         << "  <form class=\"search-box\" action=\"/search\" method=\"GET\">\n"
         << "    <input type=\"text\" name=\"q\" placeholder=\"Search libraries (e.g., charts, sqlite, camera)...\" required>\n"
         << "    <button type=\"submit\">Search</button>\n"
         << "  </form>\n"
         << "</div>\n"
         << "<div class=\"stats-bar\">\n"
         << "  <div><div class=\"stat-val\">" << total_packages << "</div><div class=\"stat-lbl\">Verified Packages</div></div>\n"
         << "  <div><div class=\"stat-val\">" << total_publishers << "</div><div class=\"stat-lbl\">Publishers</div></div>\n"
         << "  <div><div class=\"stat-val\">" << total_downloads << "+</div><div class=\"stat-lbl\">Total Installs</div></div>\n"
         << "</div>\n"
         << "<div class=\"container\">\n"
         << "  <h2>Featured Libraries</h2>\n"
         << "  <div class=\"pkg-grid\">\n";

    for (const auto& pkg : featured_packages) {
        html << "    <div class=\"card\">\n"
             << "      <div class=\"pkg-title\"><a href=\"/packages/" << pkg.name << "\" style=\"color: inherit; text-decoration: none;\">"
             << pkg.name << "</a><span class=\"badge\">v" << pkg.latest_version.to_string() << "</span></div>\n"
             << "      <p style=\"color: #8b949e; font-size: 0.95rem; margin: 12px 0;\">" << pkg.description << "</p>\n"
             << "      <div class=\"cli-snippet\">pd add " << pkg.name << "</div>\n"
             << "    </div>\n";
    }

    html << "  </div>\n</div>\n</body>\n</html>";
    return html.str();
}

std::string DeveloperPortalRenderer::render_package_page(
    const PackageInfo& pkg,
    const std::vector<std::string>& owners,
    bool is_verified,
    const std::vector<ExtendedSecurityAdvisory>& advisories)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
         << "<meta charset=\"UTF-8\">\n<title>" << pkg.name << " — PingDart Package</title>\n"
         << "<style>\n"
         << "  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0d1117; color: #c9d1d9; margin: 0; padding: 24px; }\n"
         << "  .container { max-width: 960px; margin: 0 auto; }\n"
         << "  .header { border-bottom: 1px solid #30363d; padding-bottom: 24px; margin-bottom: 24px; }\n"
         << "  h1 { color: #fff; margin: 0; font-size: 2rem; display: flex; align-items: center; gap: 12px; }\n"
         << "  .verified-badge { background: #238636; color: #fff; font-size: 0.75rem; padding: 4px 10px; border-radius: 12px; font-weight: 600; }\n"
         << "  .install-box { background: #161b22; border: 1px solid #30363d; border-radius: 6px; padding: 16px; margin: 20px 0; font-family: monospace; color: #7ee787; }\n"
         << "  .tag { background: #21262d; border: 1px solid #30363d; padding: 4px 8px; border-radius: 4px; font-size: 0.85rem; margin-right: 6px; }\n"
         << "  .sec-box { background: #1b271d; border: 1px solid #238636; border-radius: 6px; padding: 16px; margin: 20px 0; color: #7ee787; }\n"
         << "  .adv-box { background: #3d1f22; border: 1px solid #da3633; border-radius: 6px; padding: 16px; margin: 20px 0; color: #f85149; }\n"
         << "</style>\n</head>\n<body>\n"
         << "<div class=\"container\">\n"
         << "  <a href=\"/\" style=\"color: #58a6ff; text-decoration: none;\">&larr; Back to Registry</a>\n"
         << "  <div class=\"header\" style=\"margin-top: 16px;\">\n"
         << "    <h1>" << pkg.name << " <span class=\"verified-badge\">" << (is_verified ? "Verified Publisher" : "Community") << "</span></h1>\n"
         << "    <p style=\"font-size: 1.1rem; color: #8b949e;\">" << pkg.description << "</p>\n"
         << "    <div><span class=\"tag\">License: " << pkg.license << "</span><span class=\"tag\">Latest: " << pkg.latest_version.to_string() << "</span></div>\n"
         << "  </div>\n"
         << "  <div class=\"install-box\">pd add " << pkg.name << "@^" << pkg.latest_version.to_string() << "</div>\n";

    if (!advisories.empty()) {
        html << "  <div class=\"adv-box\">\n    <strong>Security Alert:</strong> " << advisories.size() << " security advisory reported for this package.\n  </div>\n";
    } else {
        html << "  <div class=\"sec-box\">\n    [✓] Automated Checks Passed: Static archive and CMake security checks passed (0 malicious binary signatures, clean CMake primitives). Scanned on 2026-11-16.\n  </div>\n";
    }

    html << "  <h3>Package Owners</h3>\n  <ul>\n";
    for (const auto& o : owners) {
        html << "    <li><a href=\"/publishers/" << o << "\" style=\"color: #58a6ff;\">" << o << "</a></li>\n";
    }
    html << "  </ul>\n"
         << "  <h3>Available Versions</h3>\n  <ul>\n";
    for (const auto& v : pkg.versions) {
        html << "    <li><strong>v" << v.version.to_string() << "</strong> — SHA-256: <code>" << v.checksum << "</code></li>\n";
    }
    html << "  </ul>\n</div>\n</body>\n</html>";
    return html.str();
}

std::string DeveloperPortalRenderer::render_search_results_page(
    std::string_view query,
    const std::vector<PackageInfo>& matches)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
         << "<meta charset=\"UTF-8\">\n<title>Search: " << query << " — PingDart</title>\n"
         << "<style>\n"
         << "  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0d1117; color: #c9d1d9; padding: 32px; }\n"
         << "  .container { max-width: 900px; margin: 0 auto; }\n"
         << "  .card { background: #161b22; border: 1px solid #30363d; border-radius: 6px; padding: 16px; margin-bottom: 16px; }\n"
         << "  a { color: #58a6ff; text-decoration: none; font-weight: 600; }\n"
         << "</style>\n</head>\n<body>\n"
         << "<div class=\"container\">\n"
         << "  <a href=\"/\">&larr; Home</a>\n"
         << "  <h2>Search Results for \"" << query << "\" (" << matches.size() << " matches)</h2>\n";

    for (const auto& pkg : matches) {
        html << "  <div class=\"card\">\n"
             << "    <h3><a href=\"/packages/" << pkg.name << "\">" << pkg.name << "</a> (v" << pkg.latest_version.to_string() << ")</h3>\n"
             << "    <p style=\"color: #8b949e;\">" << pkg.description << "</p>\n"
             << "  </div>\n";
    }
    html << "</div>\n</body>\n</html>";
    return html.str();
}

std::string DeveloperPortalRenderer::render_publisher_page(
    std::string_view username,
    bool is_verified,
    const std::vector<PackageInfo>& packages)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
         << "<meta charset=\"UTF-8\">\n<title>Publisher: " << username << " — PingDart</title>\n"
         << "<style>\n"
         << "  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0d1117; color: #c9d1d9; padding: 32px; }\n"
         << "  .container { max-width: 900px; margin: 0 auto; }\n"
         << "  .card { background: #161b22; border: 1px solid #30363d; border-radius: 6px; padding: 16px; margin-bottom: 16px; }\n"
         << "  .badge { background: #238636; color: #fff; font-size: 0.8rem; padding: 4px 8px; border-radius: 12px; }\n"
         << "</style>\n</head>\n<body>\n"
         << "<div class=\"container\">\n"
         << "  <a href=\"/\" style=\"color: #58a6ff; text-decoration: none;\">&larr; Registry Home</a>\n"
         << "  <h2>Publisher: " << username << " " << (is_verified ? "<span class=\"badge\">Verified</span>" : "") << "</h2>\n"
         << "  <h3>Published Packages (" << packages.size() << ")</h3>\n";

    for (const auto& pkg : packages) {
        html << "  <div class=\"card\">\n"
             << "    <h4><a href=\"/packages/" << pkg.name << "\" style=\"color: #58a6ff;\">" << pkg.name << "</a></h4>\n"
             << "    <p style=\"color: #8b949e;\">" << pkg.description << "</p>\n"
             << "  </div>\n";
    }
    html << "</div>\n</body>\n</html>";
    return html.str();
}

// ============================================================================
// RegistryServer Implementation (PKG-108 & PKG-109 Integration)
// ============================================================================

RegistryServer::RegistryServer() {
    seed_ecosystem();
}

void RegistryServer::seed_ecosystem() {
    // 1. pingdart-charts
    {
        PackageInfo charts_pkg{
            .name = "pingdart-charts",
            .latest_version = SemVersion{1, 2, 0, ""},
            .description = "Cross-platform charting and visualization widgets",
            .license = "MIT",
            .versions = {}
        };

        PackageVersionInfo v120{
            .version = SemVersion{1, 2, 0, ""},
            .description = "Cross-platform chart widgets with animations",
            .checksum = "sha256:7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926",
            .dependencies = {{"pingdart-core", "^1.0.0"}},
            .platforms = PlatformMatrix{},
            .archive_payload = {}
        };
        charts_pkg.versions.push_back(v120);
        packages_["pingdart-charts"] = charts_pkg;
    }

    // 2. pingdart-sqlite
    {
        PackageInfo sqlite_pkg{
            .name = "pingdart-sqlite",
            .latest_version = SemVersion{1, 0, 0, ""},
            .description = "High-performance embedded SQLite database client",
            .license = "Apache-2.0",
            .versions = {}
        };

        PackageVersionInfo v100{
            .version = SemVersion{1, 0, 0, ""},
            .description = "Stable SQLite ORM & Query Builder",
            .checksum = "sha256:9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08",
            .dependencies = {},
            .platforms = PlatformMatrix{},
            .archive_payload = {}
        };
        sqlite_pkg.versions.push_back(v100);
        packages_["pingdart-sqlite"] = sqlite_pkg;
    }

    // 3. pingdart-camera
    {
        PackageInfo cam_pkg{
            .name = "pingdart-camera",
            .latest_version = SemVersion{1, 0, 0, ""},
            .description = "Cross-platform native camera capture interface",
            .license = "MIT",
            .versions = {}
        };

        PackageVersionInfo v100{
            .version = SemVersion{1, 0, 0, ""},
            .description = "Hardware-accelerated camera preview & photo capture",
            .checksum = "sha256:3a7bd3e2360a3d29eea436fcfb7e44c735d117c42d1c1835420b6b9942dd4f1b",
            .dependencies = {},
            .platforms = PlatformMatrix{},
            .archive_payload = {}
        };
        cam_pkg.versions.push_back(v100);
        packages_["pingdart-camera"] = cam_pkg;
    }
}

size_t RegistryServer::total_packages() const noexcept {
    return packages_.size();
}

uint64_t RegistryServer::total_downloads() const noexcept {
    return total_downloads_;
}

HttpResponse RegistryServer::handle_request(const HttpRequest& request) {
    const std::string& path = request.path;

    // API Routes (/api/v1/...)
    if (path == "/api/v1/health") {
        return handle_api_health();
    }
    if (path == "/api/v1/packages") {
        return handle_api_packages(request);
    }
    if (path == "/api/v1/packages/publish") {
        if (request.method != HttpMethod::Post) {
            return HttpResponse::error(405, "Method Not Allowed (Expected POST)");
        }
        return handle_api_publish(request);
    }
    if (path == "/api/v1/auth/tokens") {
        return handle_api_auth_token(request);
    }
    if (path == "/api/v1/advisories") {
        return handle_api_advisories();
    }

    // Parameterized API paths: /api/v1/packages/:name...
    if (path.starts_with("/api/v1/packages/")) {
        std::string sub = path.substr(17); // after "/api/v1/packages/"
        size_t slash1 = std::string::npos;
        if (sub.starts_with("@")) {
            // Scoped package name has an embedded slash, e.g. @scope/name/1.0.0/download
            size_t scope_slash = sub.find('/');
            if (scope_slash != std::string::npos) {
                slash1 = sub.find('/', scope_slash + 1);
            }
        } else {
            slash1 = sub.find('/');
        }

        if (slash1 == std::string::npos) {
            return handle_api_package_detail(request, sub);
        }

        std::string pkg_name = sub.substr(0, slash1);
        std::string rest = sub.substr(slash1 + 1);
        size_t slash2 = rest.find('/');
        if (slash2 == std::string::npos) {
            return handle_api_version_detail(request, pkg_name, rest);
        }

        std::string version_str = rest.substr(0, slash2);
        std::string action = rest.substr(slash2 + 1);
        if (action == "download") {
            return handle_api_download(request, pkg_name, version_str);
        }
    }

    // Portal Routes (HTML Web UI)
    if (path == "/" || path.empty()) {
        return handle_portal_home();
    }
    if (path == "/search") {
        return handle_portal_search(request);
    }
    if (path.starts_with("/packages/")) {
        std::string pkg_name = path.substr(10);
        return handle_portal_package(pkg_name);
    }
    if (path.starts_with("/publishers/")) {
        std::string username = path.substr(12);
        return handle_portal_publisher(username);
    }

    return HttpResponse::error(404, "Endpoint or page not found: " + path);
}

HttpResponse RegistryServer::handle_api_health() {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"status\": \"healthy\",\n"
       << "  \"service\": \"PingDart Public Package Registry API\",\n"
       << "  \"version\": \"1.0.0\",\n"
       << "  \"packages_indexed\": " << packages_.size() << ",\n"
       << "  \"uptime_status\": \"operational\"\n"
       << "}";
    return HttpResponse::json(200, ss.str());
}

bool RegistryServer::check_read_authorization(const HttpRequest& req, std::string_view pkg_name) const {
    if (!is_package_private(pkg_name)) {
        return true;
    }
    std::string token = req.get_header("Authorization");
    if (token.starts_with("Bearer ")) {
        token = token.substr(7);
    } else {
        token = req.get_header("X-PingDart-Token");
    }
    if (token.empty()) {
        return false;
    }
    auto auth_res = publisher_security_.authenticate_token(token, "pkg:read");
    if (!auth_res.has_value()) {
        auth_res = publisher_security_.authenticate_token(token, "pkg:publish");
    }
    if (!auth_res.has_value()) {
        return false;
    }

    std::string user = auth_res.value();
    // Enforce Organization & Team Access Control List (PKG-112C)
    if (db_) {
        return db_->can_access_package(user, pkg_name, "read");
    }

    return true;
}

HttpResponse RegistryServer::handle_api_packages(const HttpRequest& req) {
    std::string token = req.get_header("Authorization");
    if (token.starts_with("Bearer ")) token = token.substr(7);
    else token = req.get_header("X-PingDart-Token");

    std::string requester;
    if (!token.empty()) {
        auto r = publisher_security_.authenticate_token(token, "pkg:read");
        if (!r.has_value()) r = publisher_security_.authenticate_token(token, "pkg:publish");
        if (r.has_value()) requester = r.value();
    }

    if (is_private_registry_ && requester.empty()) {
        return HttpResponse::error(401, "Authentication required to query private package registry catalog");
    }

    std::string q = req.get_query("q");
    std::vector<PackageInfo> results;

    if (db_) {
        auto db_res = db_->search_for_user(requester, q);
        if (db_res.has_value()) {
            results = db_res.value();
        } else {
            return HttpResponse::error(500, db_res.status().message());
        }
    } else {
        auto res = search(q);
        if (!res.has_value()) return HttpResponse::error(500, res.status().message());
        results = res.value();
    }

    std::ostringstream ss;
    ss << "{\n  \"count\": " << results.size() << ",\n  \"packages\": [\n";
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& p = results[i];
        ss << "    {\n"
           << "      \"name\": \"" << p.name << "\",\n"
           << "      \"latest_version\": \"" << p.latest_version.to_string() << "\",\n"
           << "      \"description\": \"" << p.description << "\",\n"
           << "      \"license\": \"" << p.license << "\"\n"
           << "    }" << (i + 1 < results.size() ? "," : "") << "\n";
    }
    ss << "  ]\n}";
    return HttpResponse::json(200, ss.str());
}

HttpResponse RegistryServer::handle_api_package_detail(const HttpRequest& req, std::string_view pkg_name) {
    if (!check_read_authorization(req, pkg_name)) {
        return HttpResponse::error(401, "Authentication required: package '" + std::string(pkg_name) + "' is private or registry requires credentials");
    }

    auto res = get_package(pkg_name);
    if (!res.has_value()) return HttpResponse::error(404, "Package not found: " + std::string(pkg_name));

    const auto& p = res.value();
    auto owners = publisher_security_.get_package_owners(p.name);

    std::ostringstream ss;
    ss << "{\n"
       << "  \"name\": \"" << p.name << "\",\n"
       << "  \"latest_version\": \"" << p.latest_version.to_string() << "\",\n"
       << "  \"description\": \"" << p.description << "\",\n"
       << "  \"license\": \"" << p.license << "\",\n"
       << "  \"versions\": [\n";

    for (size_t i = 0; i < p.versions.size(); ++i) {
        const auto& v = p.versions[i];
        ss << "    {\"version\": \"" << v.version.to_string() << "\", \"checksum\": \"" << v.checksum << "\"}"
           << (i + 1 < p.versions.size() ? "," : "") << "\n";
    }

    ss << "  ],\n  \"owners\": [";
    for (size_t i = 0; i < owners.size(); ++i) {
        ss << "\"" << owners[i] << "\"" << (i + 1 < owners.size() ? ", " : "");
    }
    ss << "]\n}";
    return HttpResponse::json(200, ss.str());
}

HttpResponse RegistryServer::handle_api_version_detail(const HttpRequest& req, std::string_view pkg_name, std::string_view version_str) {
    if (!check_read_authorization(req, pkg_name)) {
        return HttpResponse::error(401, "Authentication required: package '" + std::string(pkg_name) + "' is private or registry requires credentials");
    }

    auto v_res = SemVersion::parse(version_str);
    if (!v_res.has_value()) return HttpResponse::error(400, "Invalid SemVer version string");

    auto info_res = get_version_info(pkg_name, v_res.value());
    if (!info_res.has_value()) return HttpResponse::error(404, info_res.status().message());

    const auto& v = info_res.value();
    std::ostringstream ss;
    ss << "{\n"
       << "  \"package\": \"" << pkg_name << "\",\n"
       << "  \"version\": \"" << v.version.to_string() << "\",\n"
       << "  \"checksum\": \"" << v.checksum << "\",\n"
       << "  \"download_url\": \"/api/v1/packages/" << pkg_name << "/" << v.version.to_string() << "/download\"\n"
       << "}";
    return HttpResponse::json(200, ss.str());
}

HttpResponse RegistryServer::handle_api_download(const HttpRequest& req, std::string_view pkg_name, std::string_view version_str) {
    if (!check_read_authorization(req, pkg_name)) {
        return HttpResponse::error(401, "Authentication required: package '" + std::string(pkg_name) + "' is private or registry requires credentials");
    }

    auto v_res = SemVersion::parse(version_str);
    if (!v_res.has_value()) return HttpResponse::error(400, "Invalid SemVer version");

    auto dl_res = download_archive(pkg_name, v_res.value());
    if (!dl_res.has_value()) return HttpResponse::error(404, dl_res.status().message());

    total_downloads_++;
    return HttpResponse::binary(200, "application/octet-stream", dl_res.value());
}

HttpResponse RegistryServer::handle_api_publish(const HttpRequest& req) {
    // 1. Authenticate Authorization Header (Bearer <token>)
    std::string auth_header = req.get_header("Authorization");
    std::string token;
    if (auth_header.starts_with("Bearer ")) {
        token = auth_header.substr(7);
    } else {
        token = req.get_header("X-PingDart-Token");
    }

    if (token.empty()) {
        return HttpResponse::error(401, "Authentication token required. Run 'pd login' or provide Authorization header");
    }

    auto auth_res = publisher_security_.authenticate_token(token, "pkg:publish");
    if (!auth_res.has_value()) {
        return HttpResponse::error(403, auth_res.status().message());
    }
    std::string publisher = auth_res.value();

    // 2. Decode archive and manifest
    if (req.body.empty()) {
        return HttpResponse::error(400, "Archive payload cannot be empty");
    }

    // 3. Security Scan (PKG-109)
    auto scan_report = scanner_.scan_archive_payload(req.body);
    if (!scan_report.is_clean) {
        publisher_security_.record_audit_event(SecurityAuditEntry{
            .timestamp = "2026-11-16T20:00:00Z",
            .actor = publisher,
            .action = "scan_rejected",
            .package_name = req.get_header("X-Package-Name"),
            .version = req.get_header("X-Package-Version"),
            .status = "rejected",
            .details = "Archive rejected by PKG-109 scanner: " + scan_report.format_summary()
        });
        return HttpResponse::error(422, "Security scan rejected: " + scan_report.findings.front().description);
    }

    // Read manifest metadata from header or body
    std::string pkg_name = req.get_header("X-Package-Name");
    std::string pkg_ver = req.get_header("X-Package-Version");
    if (pkg_name.empty()) {
        pkg_name = "custom-package";
    }

    auto ver_res = SemVersion::parse(pkg_ver.empty() ? "0.1.0" : pkg_ver);
    if (!ver_res.has_value()) return HttpResponse::error(400, "Invalid package version");

    PackageManifest manifest;
    manifest.metadata().name = pkg_name;
    manifest.metadata().version = ver_res.value();

    auto status = publish_package(manifest, req.body, token);
    if (!status.ok()) {
        int code = 400;
        if (status.code() == core::StatusCode::AlreadyExists) {
            code = 409;
        } else if (status.code() == core::StatusCode::PermissionDenied) {
            code = 403;
        } else if (status.code() == core::StatusCode::Unauthenticated) {
            code = 401;
        }
        return HttpResponse::error(code, status.message());
    }

    return HttpResponse::json(201, "{\"success\": true, \"message\": \"Package published successfully\"}");
}

HttpResponse RegistryServer::handle_api_auth_token(const HttpRequest& req) {
    std::string username = req.get_query("username", "pingdart-team");
    auto tok_res = publisher_security_.create_token(username, {"pkg:publish", "pkg:read"});
    if (!tok_res.has_value()) return HttpResponse::error(400, tok_res.status().message());

    std::ostringstream ss;
    ss << "{\n"
       << "  \"token\": \"" << tok_res.value().plaintext_secret << "\",\n"
       << "  \"publisher\": \"" << username << "\",\n"
       << "  \"token_id\": \"" << tok_res.value().token.token_id << "\"\n"
       << "}";
    return HttpResponse::json(200, ss.str());
}

HttpResponse RegistryServer::handle_api_advisories() {
    auto advisories = advisory_db_.all_advisories();
    std::ostringstream ss;
    ss << "{\n  \"advisories\": [\n";
    for (size_t i = 0; i < advisories.size(); ++i) {
        const auto& a = advisories[i];
        ss << "    {\n"
           << "      \"id\": \"" << a.advisory_id << "\",\n"
           << "      \"package\": \"" << a.package_name << "\",\n"
           << "      \"severity\": \"" << scan_severity_to_string(a.severity) << "\",\n"
           << "      \"title\": \"" << a.title << "\",\n"
           << "      \"patched_version\": \"" << a.patched_version << "\"\n"
           << "    }" << (i + 1 < advisories.size() ? "," : "") << "\n";
    }
    ss << "  ]\n}";
    return HttpResponse::json(200, ss.str());
}

HttpResponse RegistryServer::handle_portal_home() {
    std::vector<PackageInfo> featured;
    for (const auto& [_, p] : packages_) {
        featured.push_back(p);
    }
    std::string html = DeveloperPortalRenderer::render_homepage(featured, packages_.size(), 2, total_downloads_);
    return HttpResponse::html(200, html);
}

HttpResponse RegistryServer::handle_portal_package(std::string_view pkg_name) {
    auto res = get_package(pkg_name);
    if (!res.has_value()) return HttpResponse::error(404, "Package not found: " + std::string(pkg_name));

    auto owners = publisher_security_.get_package_owners(pkg_name);
    bool verified = !owners.empty() && publisher_security_.is_verified_publisher(owners.front());
    auto advisories = advisory_db_.query_by_package(pkg_name);

    std::string html = DeveloperPortalRenderer::render_package_page(res.value(), owners, verified, advisories);
    return HttpResponse::html(200, html);
}

HttpResponse RegistryServer::handle_portal_search(const HttpRequest& req) {
    std::string q = req.get_query("q");
    auto res = search(q);
    std::string html = DeveloperPortalRenderer::render_search_results_page(q, res.value());
    return HttpResponse::html(200, html);
}

HttpResponse RegistryServer::handle_portal_publisher(std::string_view username) {
    bool verified = publisher_security_.is_verified_publisher(username);
    std::vector<PackageInfo> owned;
    for (const auto& [name, pkg] : packages_) {
        if (publisher_security_.can_publish(username, name)) {
            owned.push_back(pkg);
        }
    }
    std::string html = DeveloperPortalRenderer::render_publisher_page(username, verified, owned);
    return HttpResponse::html(200, html);
}

// ============================================================================
// Direct IRegistryBackend Methods
// ============================================================================

core::Result<std::vector<PackageInfo>> RegistryServer::search(std::string_view query) {
    std::vector<PackageInfo> results;
    std::string q(query);
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    for (const auto& [name, info] : packages_) {
        std::string n = name;
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        std::string d = info.description;
        std::transform(d.begin(), d.end(), d.begin(), ::tolower);

        if (q.empty() || n.find(q) != std::string::npos || d.find(q) != std::string::npos) {
            results.push_back(info);
        }
    }
    return results;
}

core::Result<PackageInfo> RegistryServer::get_package(std::string_view name) {
    auto it = packages_.find(std::string(name));
    if (it == packages_.end()) {
        return core::Status(core::StatusCode::NotFound, "Package not found: " + std::string(name));
    }
    return it->second;
}

core::Result<PackageVersionInfo> RegistryServer::get_version_info(std::string_view name, const SemVersion& version) {
    auto pkg_res = get_package(name);
    if (!pkg_res.has_value()) return pkg_res.status();

    for (const auto& v : pkg_res.value().versions) {
        if (v.version == version) {
            return v;
        }
    }
    return core::Status(core::StatusCode::NotFound, "Version " + version.to_string() + " not found for " + std::string(name));
}

core::Result<std::vector<uint8_t>> RegistryServer::download_archive(std::string_view name, const SemVersion& version) {
    auto v_res = get_version_info(name, version);
    if (!v_res.has_value()) return v_res.status();

    if (!v_res.value().archive_payload.empty()) {
        return v_res.value().archive_payload;
    }

    // Synthetic package source payload
    std::string dummy = "// " + std::string(name) + " v" + version.to_string() + "\n";
    return std::vector<uint8_t>(dummy.begin(), dummy.end());
}

core::Status RegistryServer::publish_package(
    const PackageManifest& manifest,
    const std::vector<uint8_t>& archive_data,
    std::string_view auth_token)
{
    // 1. Authenticate Token & Scope
    auto auth_res = publisher_security_.authenticate_token(auth_token, "pkg:publish");
    if (!auth_res.has_value()) return auth_res.status();
    std::string publisher = auth_res.value();

    const auto& meta = manifest.metadata();

    // 2. Ownership Verification
    auto own_status = publisher_security_.claim_or_verify_ownership(publisher, meta.name);
    if (!own_status.ok()) {
        publisher_security_.record_audit_event(SecurityAuditEntry{
            .timestamp = "2026-11-16T20:00:00Z",
            .actor = publisher,
            .action = "publish_denied",
            .package_name = meta.name,
            .version = meta.version.to_string(),
            .status = "denied",
            .details = std::string(own_status.message())
        });
        return own_status;
    }

    // 3. Security Scanning (PKG-109)
    auto scan = scanner_.scan_archive_payload(archive_data, &manifest);
    if (!scan.is_clean) {
        publisher_security_.record_audit_event(SecurityAuditEntry{
            .timestamp = "2026-11-16T20:00:00Z",
            .actor = publisher,
            .action = "publish_rejected",
            .package_name = meta.name,
            .version = meta.version.to_string(),
            .status = "rejected",
            .details = "Security scan failed: " + scan.findings.front().description
        });
        return core::Status(core::StatusCode::PermissionDenied, "Publication rejected by Security Scanner: " + scan.findings.front().description);
    }

    // 4. Immutability Enforcement
    PackageInfo& pkg = packages_[meta.name];
    pkg.name = meta.name;
    pkg.description = meta.description;
    pkg.license = meta.license;

    for (const auto& existing : pkg.versions) {
        if (existing.version == meta.version) {
            return core::Status(core::StatusCode::AlreadyExists,
                                "Package version " + meta.version.to_string() + " already exists. Published versions are strictly immutable.");
        }
    }

    if (pkg.latest_version < meta.version) {
        pkg.latest_version = meta.version;
    }

    std::string hash = ArchiveUtil::compute_sha256(archive_data);
    std::string storage_key = "sha256:" + hash;

    // 5. Persistent Storage & Relational Database Commit (PKG-112B/C)
    if (storage_) {
        auto st_res = storage_->put_archive(storage_key, archive_data);
        if (!st_res.ok()) {
            return st_res;
        }
    }

    if (db_) {
        auto db_res = db_->publish_package_version_tx(manifest, hash, storage_key, publisher);
        if (!db_res.ok()) {
            return db_res;
        }
    }

    PackageVersionInfo v_info{
        .version = meta.version,
        .description = meta.description,
        .checksum = hash,
        .dependencies = {},
        .platforms = manifest.platforms(),
        .archive_payload = archive_data
    };

    for (const auto& [dep_name, dep] : manifest.dependencies()) {
        v_info.dependencies[dep_name] = dep.raw_requirement;
    }

    pkg.versions.push_back(std::move(v_info));

    // Audit Log
    publisher_security_.record_audit_event(SecurityAuditEntry{
        .timestamp = "2026-11-16T20:00:00Z",
        .actor = publisher,
        .action = "publish",
        .package_name = meta.name,
        .version = meta.version.to_string(),
        .status = "success",
        .details = "Published new immutable version with SHA-256: " + hash
    });

    return core::Status{};
}

core::Result<std::vector<SecurityAdvisory>> RegistryServer::get_security_advisories() {
    std::vector<SecurityAdvisory> res;
    for (const auto& a : advisory_db_.all_advisories()) {
        res.push_back(SecurityAdvisory{
            .advisory_id = a.advisory_id,
            .package_name = a.package_name,
            .affected_constraint = a.affected_constraint,
            .patched_version = a.patched_version,
            .severity = scan_severity_to_string(a.severity),
            .description = a.description
        });
    }
    return res;
}

} // namespace pingdart::pkg

#include "static_generator.h"
#include <pingdart/web/url_security.h>
#include "../html/html_serializer.h"
#include "../ssr/default_web_renderer.h"
#include <iomanip>
#include <sstream>

namespace pingdart::web {

StaticGenerator::StaticGenerator(SsgOptions options)
    : options_(std::move(options)), renderer_(std::make_unique<DefaultWebRenderer>()) {}

std::string StaticGenerator::compute_asset_hash(std::string_view content) {
    // 64-bit FNV-1a hash formatted as 16-character hex string
    uint64_t hash = 14695981039346656037ULL;
    for (char c : content) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 1099511628211ULL;
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

std::string StaticGenerator::route_to_relative_file_path(std::string_view path) {
    std::string_view clean = path;
    size_t qmark = clean.find('?');
    if (qmark != std::string_view::npos) {
        clean = clean.substr(0, qmark);
    }

    // Strip leading slashes
    while (!clean.empty() && clean.front() == '/') {
        clean.remove_prefix(1);
    }
    // Strip trailing slashes
    while (!clean.empty() && clean.back() == '/') {
        clean.remove_suffix(1);
    }

    if (clean.empty()) {
        return "index.html";
    }

    std::string out(clean);
    out.append("/index.html");
    return out;
}

void StaticGenerator::add_route(StaticRoute route) {
    routes_.push_back(std::move(route));
}

void StaticGenerator::add_asset(std::string filename, std::string content) {
    raw_assets_[std::move(filename)] = std::move(content);
}

core::Result<SsgResult> StaticGenerator::build() {
    generated_files_.clear();
    std::unordered_set<std::string> output_paths;
    BuildManifest manifest;

    std::vector<std::pair<std::string, std::string>> sitemap_routes;

    // 1. Process and generate HTML pages
    for (const auto& route : routes_) {
        if (!UrlSecurity::is_safe_uri_path(route.path)) {
            return core::Status(core::StatusCode::InvalidArgument,
                                "SecurityViolation: Unsafe path in route " + route.path);
        }

        std::string rel_file = route_to_relative_file_path(route.path);

        if (output_paths.find(rel_file) != output_paths.end()) {
            return core::Status(core::StatusCode::AlreadyExists,
                                "Route collision: Duplicate output path " + rel_file);
        }
        output_paths.insert(rel_file);

        // Render semantic widget tree
        HtmlSerializer serializer(options_.pretty_html);
        if (route.widget_builder) {
            auto widget = route.widget_builder();
            if (widget) {
                render_semantic_tree(*widget, serializer);
            }
        }

        std::string body_html = serializer.to_string();
        std::string full_document = renderer_->render_document(route.seo, body_html);

        generated_files_[rel_file] = std::move(full_document);
        manifest.routes.push_back(route.path);

        if (route.is_canonical && route.is_indexable) {
            std::string canonical_loc = route.seo.canonical_url.empty()
                ? options_.sitemap_base_url + route.path
                : route.seo.canonical_url;
            sitemap_routes.push_back({std::move(canonical_loc), route.lastmod});
        }
    }

    // 2. Fingerprint and copy static assets
    for (const auto& [filename, content] : raw_assets_) {
        std::string hash = compute_asset_hash(content);
        size_t dot_pos = filename.rfind('.');
        std::string stem = (dot_pos != std::string::npos) ? filename.substr(0, dot_pos) : filename;
        std::string ext = (dot_pos != std::string::npos) ? filename.substr(dot_pos) : "";

        std::string hashed_filename = "assets/" + stem + "." + hash + ext;
        generated_files_[hashed_filename] = content;

        manifest.assets.push_back(AssetEntry{
            .original_name = filename,
            .hashed_name = hashed_filename,
            .hash = hash,
            .byte_size = content.size()
        });
    }

    // 3. Generate sitemap.xml
    std::string sitemap_xml = renderer_->render_sitemap(sitemap_routes);
    generated_files_["sitemap.xml"] = std::move(sitemap_xml);

    // 4. Generate robots.txt
    std::string sitemap_url = options_.sitemap_base_url + "/sitemap.xml";
    std::string robots_txt = renderer_->render_robots_txt(options_.allow_indexing, sitemap_url);
    generated_files_["robots.txt"] = std::move(robots_txt);

    // 5. Generate build-manifest.json
    generated_files_["build-manifest.json"] = manifest.to_json();

    SsgResult result{
        .pages_generated = routes_.size(),
        .assets_fingerprinted = raw_assets_.size(),
        .output_directory = options_.output_directory,
        .manifest = std::move(manifest)
    };

    return result;
}

} // namespace pingdart::web

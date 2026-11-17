// include/pingcore/router.h
// Next.js-Style File-Based Routing System for PingCore C++ Applications (PKG-119)
#pragma once

#include <pingcore/ui.h>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace pingcore {

using PageFactory = std::function<Widget()>;
using LayoutFactory = std::function<Widget(Widget)>;
using RouteListener = std::function<void(std::string_view)>;

struct RouteMatch {
    bool matched{false};
    std::string pattern;
    std::unordered_map<std::string, std::string> params;
};

class Router {
public:
    static Router& instance() {
        static Router s_instance;
        return s_instance;
    }

    Router() {
        // Default 404 Not Found fallback
        not_found_factory_ = []() -> Widget {
            return Center(
                Column({
                    Text("404").fontSize(48.0f).bold().color(Color::rgb(239, 68, 68)),
                    Text("This page could not be found.").fontSize(18.0f).color(Color::rgb(148, 163, 184)),
                    Button("Return Home").onClick([] {
                        Router::instance().navigate("/");
                    })
                }).spacing(16.0f)
            );
        };

        // Default Loading fallback
        loading_factory_ = []() -> Widget {
            return Center(
                Text("Loading...").fontSize(20.0f).color(Color::rgb(148, 163, 184))
            );
        };
    }

    void register_page(std::string path, PageFactory page_factory) {
        // Normalize path
        if (path.empty() || path[0] != '/') {
            path = "/" + path;
        }
        page_routes_.push_back(PageRouteEntry{std::move(path), std::move(page_factory)});
    }

    void register_layout(std::string path_prefix, LayoutFactory layout_factory) {
        if (path_prefix.empty() || path_prefix[0] != '/') {
            path_prefix = "/" + path_prefix;
        }
        layout_routes_.push_back(LayoutRouteEntry{std::move(path_prefix), std::move(layout_factory)});
    }

    void register_not_found(PageFactory not_found_factory) {
        not_found_factory_ = std::move(not_found_factory);
    }

    void register_loading(PageFactory loading_factory) {
        loading_factory_ = std::move(loading_factory);
    }

    void navigate(std::string_view target_path) {
        std::string norm_path = std::string(target_path);
        if (norm_path.empty() || norm_path[0] != '/') {
            norm_path = "/" + norm_path;
        }

        current_path_ = norm_path;
        current_params_.clear();

        // Check matching route and populate params
        for (const auto& entry : page_routes_) {
            auto match = match_route(entry.pattern, current_path_);
            if (match.matched) {
                current_params_ = match.params;
                break;
            }
        }

        for (const auto& listener : listeners_) {
            listener(current_path_);
        }
    }

    [[nodiscard]] std::string_view current_path() const noexcept {
        return current_path_;
    }

    [[nodiscard]] std::string param(std::string_view key) const {
        auto it = current_params_.find(std::string(key));
        return (it != current_params_.end()) ? it->second : "";
    }

    [[nodiscard]] const std::unordered_map<std::string, std::string>& params() const noexcept {
        return current_params_;
    }

    [[nodiscard]] Widget render_current_page() const {
        const PageRouteEntry* matched_page = nullptr;
        std::unordered_map<std::string, std::string> matched_params;

        for (const auto& entry : page_routes_) {
            auto match = match_route(entry.pattern, current_path_);
            if (match.matched) {
                matched_page = &entry;
                matched_params = match.params;
                break;
            }
        }

        Widget content;
        if (matched_page && matched_page->factory) {
            content = matched_page->factory();
        } else if (not_found_factory_) {
            content = not_found_factory_();
        }

        if (!content) {
            return nullptr;
        }

        // Apply matching layouts hierarchically from root down to deepest section
        // Sort matching layouts by path length ascending (root first)
        std::vector<LayoutFactory> applicable_layouts;
        for (const auto& l_entry : layout_routes_) {
            if (is_prefix_match(l_entry.prefix, current_path_)) {
                applicable_layouts.push_back(l_entry.factory);
            }
        }

        // Wrap content with layouts from innermost to outermost
        for (auto it = applicable_layouts.rbegin(); it != applicable_layouts.rend(); ++it) {
            content = (*it)(content);
        }

        return content;
    }

    void add_listener(RouteListener listener) {
        listeners_.push_back(std::move(listener));
    }

    void clear() {
        page_routes_.clear();
        layout_routes_.clear();
        listeners_.clear();
        current_path_ = "/";
        current_params_.clear();
    }

    static RouteMatch match_route(const std::string& pattern, const std::string& actual_path) {
        if (pattern == actual_path) {
            return RouteMatch{true, pattern, {}};
        }

        auto pat_parts = split(pattern, '/');
        auto act_parts = split(actual_path, '/');

        if (pat_parts.size() != act_parts.size()) {
            return RouteMatch{false, pattern, {}};
        }

        std::unordered_map<std::string, std::string> params;
        for (size_t i = 0; i < pat_parts.size(); ++i) {
            if (pat_parts[i].starts_with(':')) {
                params[pat_parts[i].substr(1)] = act_parts[i];
            } else if (pat_parts[i].starts_with('[') && pat_parts[i].ends_with(']')) {
                // Next.js style [id]
                params[pat_parts[i].substr(1, pat_parts[i].size() - 2)] = act_parts[i];
            } else if (pat_parts[i] != act_parts[i]) {
                return RouteMatch{false, pattern, {}};
            }
        }

        return RouteMatch{true, pattern, std::move(params)};
    }

    static bool is_prefix_match(const std::string& prefix, const std::string& actual_path) {
        if (prefix == "/" || prefix.empty()) return true;
        if (actual_path == prefix) return true;
        if (actual_path.starts_with(prefix + "/")) return true;
        return false;
    }

    static std::vector<std::string> split(std::string_view s, char delim) {
        std::vector<std::string> parts;
        size_t start = 0;
        while (start < s.size()) {
            size_t end = s.find(delim, start);
            if (end == std::string_view::npos) {
                if (start < s.size()) parts.emplace_back(s.substr(start));
                break;
            }
            if (end > start) {
                parts.emplace_back(s.substr(start, end - start));
            }
            start = end + 1;
        }
        return parts;
    }

private:
    struct PageRouteEntry {
        std::string pattern;
        PageFactory factory;
    };

    struct LayoutRouteEntry {
        std::string prefix;
        LayoutFactory factory;
    };

    std::vector<PageRouteEntry> page_routes_;
    std::vector<LayoutRouteEntry> layout_routes_;
    PageFactory not_found_factory_;
    PageFactory loading_factory_;
    std::string current_path_{"/"};
    std::unordered_map<std::string, std::string> current_params_;
    std::vector<RouteListener> listeners_;
};

// Global navigation helper implementation
inline void navigate(std::string_view path) {
    Router::instance().navigate(path);
}

inline Router& router() {
    return Router::instance();
}

// Next.js-Style File-Based Route Scanner & Code Generator
class RouteScanner {
public:
    struct DiscoveredPage {
        std::string file_path;
        std::string route_path; // e.g. "/", "/login", "/users/:id"
    };

    struct DiscoveredLayout {
        std::string file_path;
        std::string route_prefix; // e.g. "/", "/dashboard"
    };

    struct DiscoveredApiRoute {
        std::string file_path;
        std::string api_path; // e.g. "/api/users"
    };

    struct ScanResult {
        std::vector<DiscoveredPage> pages;
        std::vector<DiscoveredLayout> layouts;
        std::vector<DiscoveredApiRoute> api_routes;
        std::string loading_file;
        std::string not_found_file;
    };

    static ScanResult scan(const std::filesystem::path& app_dir) {
        ScanResult result;
        if (!std::filesystem::exists(app_dir) || !std::filesystem::is_directory(app_dir)) {
            return result;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(app_dir)) {
            if (!entry.is_regular_file()) continue;

            auto path = entry.path();
            std::string filename = path.filename().string();
            if (filename != "page.cpp" && filename != "layout.cpp" &&
                filename != "loading.cpp" && filename != "not_found.cpp" &&
                filename != "route.cpp") {
                continue;
            }

            auto rel = std::filesystem::relative(path, app_dir);
            auto parent = rel.parent_path().string();

            // Convert directory path to route path
            // Replace Next.js style [param] with :param
            std::string route = to_route_path(parent);

            if (filename == "page.cpp") {
                result.pages.push_back(DiscoveredPage{path.string(), route});
            } else if (filename == "layout.cpp") {
                result.layouts.push_back(DiscoveredLayout{path.string(), route});
            } else if (filename == "loading.cpp" && parent.empty()) {
                result.loading_file = path.string();
            } else if (filename == "not_found.cpp" && parent.empty()) {
                result.not_found_file = path.string();
            } else if (filename == "route.cpp" && rel.string().starts_with("api")) {
                result.api_routes.push_back(DiscoveredApiRoute{path.string(), "/" + rel.parent_path().string()});
            }
        }

        return result;
    }

    static std::string to_route_path(const std::string& dir_path) {
        if (dir_path.empty() || dir_path == ".") {
            return "/";
        }

        std::vector<std::string> parts = Router::split(dir_path, '/');
        std::string res;
        for (const auto& part : parts) {
            res += "/";
            if (part.starts_with('[') && part.ends_with(']')) {
                res += ":" + part.substr(1, part.size() - 2);
            } else {
                res += part;
            }
        }
        return res.empty() ? "/" : res;
    }
};

} // namespace pingcore

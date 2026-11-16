#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace pingdart::web {

struct RouteMatch {
    bool matched{false};
    std::string route_pattern;
    std::string matched_path;
    std::vector<std::pair<std::string, std::string>> params;

    [[nodiscard]] std::string_view get_param(std::string_view key, std::string_view fallback = "") const noexcept {
        for (const auto& [k, v] : params) {
            if (k == key) {
                return v;
            }
        }
        return fallback;
    }
};

class IRouter {
public:
    virtual ~IRouter() = default;

    [[nodiscard]] virtual RouteMatch match(std::string_view request_path) const = 0;
    virtual void navigate(std::string_view target_path) = 0;
    [[nodiscard]] virtual std::string_view current_path() const noexcept = 0;
};

} // namespace pingdart::web

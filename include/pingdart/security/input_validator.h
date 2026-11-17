// include/pingdart/security/input_validator.h
// PKG-115: Input Sanitization and Boundary Validation
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <pingdart/core/result.h>

namespace pingdart::security {

struct PinnedConnectionTarget {
    std::string original_host;
    std::string pinned_ip_address;
    uint16_t port{443};
    bool is_tls{true};
    std::string request_path{"/"};
};

class InputValidator {
public:
    /// Validates that candidate_path does not escape root_dir via path traversal (..), symlinks, or absolute prefixes
    [[nodiscard]] static core::Result<std::filesystem::path> validate_path_within_root(
        const std::filesystem::path& candidate_path,
        const std::filesystem::path& root_dir
    );

    /// Checks if a string contains null bytes or path traversal sequences
    [[nodiscard]] static bool has_path_traversal(std::string_view path_str) noexcept;

    /// Checks and sanitizes shell command arguments against metacharacter injection
    [[nodiscard]] static core::Result<std::string> sanitize_shell_argument(std::string_view arg);

    /// Validates URL string and optionally blocks private IP ranges and localhost (SSRF defense)
    [[nodiscard]] static core::Status validate_url(
        std::string_view url_str,
        bool allow_private_ips = false
    );

    /// Checks if an IPv4 or IPv6 string belongs to loopback, RFC1918 private subnets, link-local metadata, or reserved ranges
    [[nodiscard]] static bool is_private_or_restricted_ip(std::string_view ip_address) noexcept;

    /// Connection-time SSRF & DNS rebinding defense: resolves host and validates every returned IP address
    [[nodiscard]] static core::Status validate_socket_destination(
        std::string_view host,
        uint16_t port = 443,
        bool allow_private_ips = false
    );

    /// Resolves target URL, validates against SSRF/rebinding, and pins the exact IP address for socket connect
    [[nodiscard]] static core::Result<PinnedConnectionTarget> resolve_and_pin_destination(
        std::string_view url_str,
        bool allow_private_ips = false
    );

    /// Validates HTTP redirect to prevent protocol downgrade, infinite loops, and redirect-based SSRF
    [[nodiscard]] static core::Status validate_redirect(
        std::string_view source_url,
        std::string_view target_url,
        int current_redirect_count = 1,
        int max_redirects = 5
    );

    /// Validates redirect and pins the verified destination IP for subsequent socket connection
    [[nodiscard]] static core::Result<PinnedConnectionTarget> validate_and_pin_redirect(
        std::string_view source_url,
        std::string_view target_url,
        int current_redirect_count = 1,
        int max_redirects = 5
    );

    /// Validates package names and system identifiers
    [[nodiscard]] static bool is_safe_identifier(std::string_view identifier) noexcept;
};

} // namespace pingdart::security

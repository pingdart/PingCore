// src/security/input_validator.cpp
// PKG-115: Input Sanitization and Boundary Validation Implementation
#include <pingdart/security/input_validator.h>
#include <algorithm>
#include <cctype>
#include <cstring>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

namespace pingdart::security {

bool InputValidator::has_path_traversal(std::string_view path_str) noexcept {
    if (path_str.find('\0') != std::string_view::npos) return true;
    if (path_str.find("..") != std::string_view::npos) return true;
    return false;
}

core::Result<std::filesystem::path> InputValidator::validate_path_within_root(
    const std::filesystem::path& candidate_path,
    const std::filesystem::path& root_dir)
{
    std::string candidate_str = candidate_path.string();
    if (has_path_traversal(candidate_str)) {
        return core::Status(core::StatusCode::InvalidArgument, "Path traversal sequence detected in: " + candidate_str);
    }

    std::error_code ec;
    std::filesystem::path abs_root = std::filesystem::weakly_canonical(root_dir, ec);
    if (ec) {
        return core::Status(core::StatusCode::InvalidArgument, "Invalid root directory: " + ec.message());
    }

    std::filesystem::path full_path = candidate_path.is_absolute()
        ? candidate_path
        : abs_root / candidate_path;

    std::filesystem::path abs_candidate = std::filesystem::weakly_canonical(full_path, ec);
    if (ec) {
        return core::Status(core::StatusCode::InvalidArgument, "Cannot resolve path: " + ec.message());
    }

    // Verify candidate begins with root prefix
    std::string root_s = abs_root.string();
    std::string cand_s = abs_candidate.string();

    if (cand_s.size() < root_s.size() ||
        cand_s.compare(0, root_s.size(), root_s) != 0) {
        return core::Status(core::StatusCode::PermissionDenied, "Path escapes designated root directory: " + cand_s);
    }

    if (cand_s.size() > root_s.size() &&
        cand_s[root_s.size()] != '/' &&
        cand_s[root_s.size()] != '\\') {
        return core::Status(core::StatusCode::PermissionDenied, "Path does not reside within directory boundary: " + cand_s);
    }

    return abs_candidate;
}

core::Result<std::string> InputValidator::sanitize_shell_argument(std::string_view arg) {
    if (arg.empty()) {
        return std::string("''");
    }

    for (char c : arg) {
        if (c == '\0' || c == '\n' || c == '\r' || c == ';' || c == '&' ||
            c == '|' || c == '$' || c == '`' || c == '>' || c == '<' ||
            c == '(' || c == ')' || c == '{' || c == '}') {
            return core::Status(core::StatusCode::InvalidArgument, "Dangerous shell metacharacter detected in argument");
        }
    }

    std::string escaped = "'";
    for (char c : arg) {
        if (c == '\'') {
            escaped += "'\\''";
        } else {
            escaped += c;
        }
    }
    escaped += "'";
    return escaped;
}

core::Status InputValidator::validate_url(std::string_view url_str, bool allow_private_ips) {
    if (url_str.empty()) {
        return core::Status(core::StatusCode::InvalidArgument, "URL cannot be empty");
    }

    std::string_view scheme;
    std::string_view rest;
    if (url_str.starts_with("https://")) {
        scheme = "https";
        rest = url_str.substr(8);
    } else if (url_str.starts_with("http://")) {
        scheme = "http";
        rest = url_str.substr(7);
    } else {
        return core::Status(core::StatusCode::InvalidArgument, "URL scheme must be http or https");
    }

    if (rest.empty()) {
        return core::Status(core::StatusCode::InvalidArgument, "Missing host in URL");
    }

    // Extract host portion up to '/' or ':' or '?'
    auto host_end = rest.find_first_of("/:?#");
    std::string_view host = (host_end == std::string_view::npos) ? rest : rest.substr(0, host_end);

    if (host.empty()) {
        return core::Status(core::StatusCode::InvalidArgument, "Empty host in URL");
    }

    if (!allow_private_ips) {
        if (host == "localhost" || host == "127.0.0.1" || host == "::1" ||
            host == "0.0.0.0" || host == "169.254.169.254" || host == "[::1]") {
            return core::Status(core::StatusCode::PermissionDenied, "SSRF Prevention: Localhost / loopback destination denied");
        }
        if (host.starts_with("10.") || host.starts_with("192.168.") ||
            host.starts_with("172.16.") || host.starts_with("172.17.") ||
            host.starts_with("172.18.") || host.starts_with("172.19.") ||
            host.starts_with("172.2") || host.starts_with("172.30.") ||
            host.starts_with("172.31.")) {
            return core::Status(core::StatusCode::PermissionDenied, "SSRF Prevention: Private subnet destination denied");
        }
    }

    return core::Status::Ok();
}

bool InputValidator::is_private_or_restricted_ip(std::string_view ip_address) noexcept {
    if (ip_address.empty()) return true;

    // Direct string checks for well-known loopback & cloud metadata hosts
    if (ip_address == "localhost" || ip_address == "127.0.0.1" || ip_address == "::1" ||
        ip_address == "0.0.0.0" || ip_address == "169.254.169.254" || ip_address == "[::1]" ||
        ip_address == "metadata.google.internal" || ip_address == "instance-data") {
        return true;
    }

    std::string ip_str(ip_address);

    // Strip enclosing brackets if [IPv6] format
    if (ip_str.front() == '[' && ip_str.back() == ']') {
        ip_str = ip_str.substr(1, ip_str.size() - 2);
    }

    // 1. Check IPv4
    struct in_addr addr4;
    if (inet_pton(AF_INET, ip_str.c_str(), &addr4) == 1) {
        uint32_t ip = ntohl(addr4.s_addr);
        if ((ip >> 24) == 0) return true;      // 0.0.0.0/8 Current network
        if ((ip >> 24) == 10) return true;     // 10.0.0.0/8 RFC1918 Private
        if ((ip >> 24) == 127) return true;    // 127.0.0.0/8 Loopback
        if ((ip >> 16) == 0xA9FE) return true; // 169.254.0.0/16 Link-Local / Metadata
        if ((ip >> 20) == 0xAC1) return true;  // 172.16.0.0/12 RFC1918 Private
        if ((ip >> 16) == 0xC0A8) return true; // 192.168.0.0/16 RFC1918 Private
        if ((ip >> 22) == 0x644) return true;  // 100.64.0.0/10 Carrier-grade NAT
        if ((ip >> 28) == 14) return true;     // 224.0.0.0/4 Multicast
        if ((ip >> 28) == 15) return true;     // 240.0.0.0/4 Reserved
        if (ip == 0xFFFFFFFF) return true;     // Broadcast
        return false;
    }

    // 2. Check IPv6
    struct in6_addr addr6;
    if (inet_pton(AF_INET6, ip_str.c_str(), &addr6) == 1) {
        const uint8_t* b = addr6.s6_addr;
        // ::1 loopback
        bool is_loopback = true;
        for (int i = 0; i < 15; ++i) {
            if (b[i] != 0) { is_loopback = false; break; }
        }
        if (is_loopback && b[15] == 1) return true;

        // :: unspecified
        bool is_unspecified = true;
        for (int i = 0; i < 16; ++i) {
            if (b[i] != 0) { is_unspecified = false; break; }
        }
        if (is_unspecified) return true;

        // fc00::/7 Unique Local Address (ULA)
        if ((b[0] & 0xFE) == 0xFC) return true;

        // fe80::/10 Link-Local
        if (b[0] == 0xFE && (b[1] & 0xC0) == 0x80) return true;

        // ff00::/8 Multicast
        if (b[0] == 0xFF) return true;

        // ::ffff:0:0/96 IPv4-mapped IPv6
        bool is_v4_mapped = true;
        for (int i = 0; i < 10; ++i) {
            if (b[i] != 0) { is_v4_mapped = false; break; }
        }
        if (is_v4_mapped && b[10] == 0xFF && b[11] == 0xFF) {
            uint32_t inner_v4 = (static_cast<uint32_t>(b[12]) << 24) |
                                (static_cast<uint32_t>(b[13]) << 16) |
                                (static_cast<uint32_t>(b[14]) << 8) |
                                static_cast<uint32_t>(b[15]);
            if ((inner_v4 >> 24) == 127) return true;
            if ((inner_v4 >> 24) == 10) return true;
            if ((inner_v4 >> 16) == 0xA9FE) return true;
            if ((inner_v4 >> 20) == 0xAC1) return true;
            if ((inner_v4 >> 16) == 0xC0A8) return true;
        }

        return false;
    }

    return false;
}

core::Status InputValidator::validate_socket_destination(
    std::string_view host,
    uint16_t port,
    bool allow_private_ips)
{
    (void)port;
    if (host.empty()) {
        return core::Status(core::StatusCode::InvalidArgument, "Host cannot be empty");
    }

    if (!allow_private_ips && is_private_or_restricted_ip(host)) {
        return core::Status(core::StatusCode::PermissionDenied,
            "SSRF / DNS Rebinding Prevention: Host is in a restricted network range");
    }

    if (!allow_private_ips) {
        std::string host_str(host);
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        struct addrinfo* res = nullptr;
        int status = getaddrinfo(host_str.c_str(), nullptr, &hints, &res);
        if (status == 0 && res != nullptr) {
            for (struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
                char ip_buf[INET6_ADDRSTRLEN];
                if (p->ai_family == AF_INET) {
                    struct sockaddr_in* s4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
                    inet_ntop(AF_INET, &s4->sin_addr, ip_buf, sizeof(ip_buf));
                } else if (p->ai_family == AF_INET6) {
                    struct sockaddr_in6* s6 = reinterpret_cast<struct sockaddr_in6*>(p->ai_addr);
                    inet_ntop(AF_INET6, &s6->sin6_addr, ip_buf, sizeof(ip_buf));
                } else {
                    continue;
                }

                if (is_private_or_restricted_ip(ip_buf)) {
                    freeaddrinfo(res);
                    return core::Status(core::StatusCode::PermissionDenied,
                        std::string("SSRF / DNS Rebinding Prevention: Resolved destination address is restricted: ") + ip_buf);
                }
            }
            freeaddrinfo(res);
        }
    }

    return core::Status::Ok();
}

core::Result<PinnedConnectionTarget> InputValidator::resolve_and_pin_destination(
    std::string_view url_str,
    bool allow_private_ips)
{
    auto url_st = validate_url(url_str, allow_private_ips);
    if (!url_st.ok()) {
        return url_st;
    }

    bool is_tls = url_str.starts_with("https://");
    std::string_view rest = is_tls ? url_str.substr(8) : url_str.substr(7);

    auto host_end = rest.find_first_of("/:?#");
    std::string_view host_part = (host_end == std::string_view::npos) ? rest : rest.substr(0, host_end);
    std::string host(host_part);

    uint16_t port = is_tls ? 443 : 80;
    if (host_end != std::string_view::npos && rest[host_end] == ':') {
        auto port_end = rest.find_first_of("/?#", host_end + 1);
        std::string_view port_str = (port_end == std::string_view::npos)
            ? rest.substr(host_end + 1)
            : rest.substr(host_end + 1, port_end - (host_end + 1));
        try {
            port = static_cast<uint16_t>(std::stoul(std::string(port_str)));
        } catch (...) {
            return core::Status(core::StatusCode::InvalidArgument, "Invalid port in URL");
        }
    }

    std::string req_path = "/";
    auto path_start = rest.find_first_of("/?#");
    if (path_start != std::string_view::npos) {
        req_path = std::string(rest.substr(path_start));
    }

    if (!allow_private_ips && is_private_or_restricted_ip(host)) {
        return core::Status(core::StatusCode::PermissionDenied,
            "SSRF / DNS Rebinding Prevention: Host is in a restricted network range");
    }

#if defined(_WIN32)
    static bool s_winsock_init = false;
    if (!s_winsock_init) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        s_winsock_init = true;
    }
#endif

    std::string pinned_ip;
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    int status = getaddrinfo(host.c_str(), nullptr, &hints, &res);
    if (status != 0 || res == nullptr) {
        return core::Status(core::StatusCode::Unavailable, "Failed to resolve hostname: " + host);
    }

    for (struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
        char ip_buf[INET6_ADDRSTRLEN];
        if (p->ai_family == AF_INET) {
            struct sockaddr_in* s4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
            inet_ntop(AF_INET, &s4->sin_addr, ip_buf, sizeof(ip_buf));
        } else if (p->ai_family == AF_INET6) {
            struct sockaddr_in6* s6 = reinterpret_cast<struct sockaddr_in6*>(p->ai_addr);
            inet_ntop(AF_INET6, &s6->sin6_addr, ip_buf, sizeof(ip_buf));
        } else {
            continue;
        }

        if (!allow_private_ips && is_private_or_restricted_ip(ip_buf)) {
            freeaddrinfo(res);
            return core::Status(core::StatusCode::PermissionDenied,
                std::string("SSRF / DNS Rebinding Prevention: Resolved destination address is restricted: ") + ip_buf);
        }

        if (pinned_ip.empty()) {
            pinned_ip = ip_buf;
        }
    }
    freeaddrinfo(res);

    if (pinned_ip.empty()) {
        return core::Status(core::StatusCode::Unavailable, "No valid IP addresses resolved for host: " + host);
    }

    PinnedConnectionTarget target;
    target.original_host = host;
    target.pinned_ip_address = pinned_ip;
    target.port = port;
    target.is_tls = is_tls;
    target.request_path = req_path;
    return target;
}

core::Status InputValidator::validate_redirect(
    std::string_view source_url,
    std::string_view target_url,
    int current_redirect_count,
    int max_redirects)
{
    auto res = validate_and_pin_redirect(source_url, target_url, current_redirect_count, max_redirects);
    return res.status();
}

core::Result<PinnedConnectionTarget> InputValidator::validate_and_pin_redirect(
    std::string_view source_url,
    std::string_view target_url,
    int current_redirect_count,
    int max_redirects)
{
    if (current_redirect_count > max_redirects) {
        return core::Status(core::StatusCode::ResourceExhausted, "Max redirect chain limit exceeded");
    }

    // Protocol downgrade protection: https -> http disallowed
    if (source_url.starts_with("https://") && target_url.starts_with("http://")) {
        return core::Status(core::StatusCode::PermissionDenied,
            "Insecure protocol downgrade rejected on redirect: https -> http");
    }

    return resolve_and_pin_destination(target_url, false);
}

bool InputValidator::is_safe_identifier(std::string_view identifier) noexcept {
    if (identifier.empty() || identifier.size() > 128) return false;
    char first = identifier.front();
    if (first == '.' || first == '-' || first == '_') return false;

    for (char c : identifier) {
        if (!std::isalnum(static_cast<unsigned char>(c)) &&
            c != '_' && c != '-' && c != '.') {
            return false;
        }
    }
    return true;
}

} // namespace pingdart::security

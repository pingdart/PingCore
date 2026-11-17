// include/pingcore/server.h
// PingCore Backend API SDK (PKG-126: HTTP Security, Request Smuggling Hardening,
// RFC 6455 WebSocket Fragmentation/UTF-8 & Backpressure)
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <sstream>
#include <atomic>
#include <thread>
#include <mutex>
#include <cstdint>
#include <cstring>
#include <optional>
#include <algorithm>
#include <chrono>
#include <deque>
#include <cctype>
#include <pingdart/core/result.h>
#include <pingdart/core/status.h>

namespace pingcore {

struct JsonValue {
    enum class Type { String, Bool, Number };
    Type type{Type::String};
    std::string str_val;
    bool bool_val{false};
    double num_val{0.0};

    JsonValue(const char* s) : type(Type::String), str_val(s) {}
    JsonValue(std::string s) : type(Type::String), str_val(std::move(s)) {}
    JsonValue(bool b) : type(Type::Bool), bool_val(b) {}
    JsonValue(int n) : type(Type::Number), num_val(n) {}
    JsonValue(double n) : type(Type::Number), num_val(n) {}

    [[nodiscard]] std::string to_json() const {
        if (type == Type::String) return "\"" + str_val + "\"";
        if (type == Type::Bool) return bool_val ? "true" : "false";
        if (type == Type::Number) {
            if (num_val == static_cast<double>(static_cast<long long>(num_val))) {
                return std::to_string(static_cast<long long>(num_val));
            }
            return std::to_string(num_val);
        }
        return "null";
    }
};

class Request {
public:
    Request() = default;
    Request(std::string method, std::string path, std::string body = "")
        : method_(std::move(method)), path_(std::move(path)), body_(std::move(body)) {}

    [[nodiscard]] const std::string& method() const noexcept { return method_; }
    [[nodiscard]] const std::string& path() const noexcept { return path_; }
    [[nodiscard]] const std::string& body() const noexcept { return body_; }

    void set_header(std::string key, std::string value) {
        headers_[std::move(key)] = std::move(value);
    }

    [[nodiscard]] std::string header(std::string_view key) const {
        auto it = headers_.find(std::string(key));
        return (it != headers_.end()) ? it->second : "";
    }

    void set_param(std::string key, std::string value) {
        params_[std::move(key)] = std::move(value);
    }

    [[nodiscard]] std::string param(std::string_view key) const {
        auto it = params_.find(std::string(key));
        return (it != params_.end()) ? it->second : "";
    }

    void set_body(std::string body) { body_ = std::move(body); }

private:
    std::string method_{"GET"};
    std::string path_{"/"};
    std::string body_;
    std::unordered_map<std::string, std::string> headers_;
    std::unordered_map<std::string, std::string> params_;
};

class Response {
public:
    Response() = default;

    Response& status(int code) {
        status_code_ = code;
        return *this;
    }

    Response& header(std::string key, std::string value) {
        headers_[std::move(key)] = std::move(value);
        return *this;
    }

    Response& set_header(std::string key, std::string value) {
        return header(std::move(key), std::move(value));
    }

    Response& send(std::string_view body) {
        body_ = std::string(body);
        if (headers_.find("Content-Type") == headers_.end()) {
            headers_["Content-Type"] = "text/plain; charset=utf-8";
        }
        return *this;
    }

    Response& json(std::string_view json_str) {
        body_ = std::string(json_str);
        headers_["Content-Type"] = "application/json; charset=utf-8";
        return *this;
    }

    Response& sse(std::string_view data, std::string_view event = "") {
        status_code_ = 200;
        headers_["Content-Type"] = "text/event-stream; charset=utf-8";
        headers_["Cache-Control"] = "no-cache";
        headers_["Connection"] = "keep-alive";

        std::ostringstream ss;
        if (!event.empty()) {
            ss << "event: " << event << "\n";
        }
        ss << "data: " << data << "\n\n";
        body_ = ss.str();
        return *this;
    }

    static Response json(std::initializer_list<std::pair<std::string, JsonValue>> fields) {
        std::ostringstream ss;
        ss << "{";
        bool first = true;
        for (const auto& [k, v] : fields) {
            if (!first) ss << ",";
            first = false;
            ss << "\"" << k << "\":" << v.to_json();
        }
        ss << "}";
        Response res;
        res.json(ss.str());
        return res;
    }

    [[nodiscard]] int status_code() const noexcept { return status_code_; }
    [[nodiscard]] const std::string& body() const noexcept { return body_; }
    [[nodiscard]] const std::unordered_map<std::string, std::string>& headers() const noexcept { return headers_; }

    [[nodiscard]] std::string to_http_wire_format() const {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << status_code_ << " ";
        switch (status_code_) {
            case 200: ss << "OK"; break;
            case 201: ss << "Created"; break;
            case 204: ss << "No Content"; break;
            case 400: ss << "Bad Request"; break;
            case 401: ss << "Unauthorized"; break;
            case 403: ss << "Forbidden"; break;
            case 404: ss << "Not Found"; break;
            case 413: ss << "Payload Too Large"; break;
            case 414: ss << "URI Too Long"; break;
            case 431: ss << "Request Header Fields Too Large"; break;
            case 500: ss << "Internal Server Error"; break;
            default:  ss << "Status"; break;
        }
        ss << "\r\n";
        for (const auto& [k, v] : headers_) {
            ss << k << ": " << v << "\r\n";
        }
        ss << "Content-Length: " << body_.size() << "\r\n";
        ss << "\r\n";
        ss << body_;
        return ss.str();
    }

private:
    int status_code_{200};
    std::string body_;
    std::unordered_map<std::string, std::string> headers_;
};

using Next = std::function<void()>;
using Middleware = std::function<void(Request&, Response&, Next)>;
using RouteHandler = std::function<void(const Request&, Response&)>;

inline bool is_websocket_handshake(const Request& req) {
    std::string up = req.header("Upgrade");
    return (up.find("websocket") != std::string::npos || up.find("WebSocket") != std::string::npos);
}

// ============================================================================
// PKG-126/127: HTTP Security, Framing Limits, Pipelining & Request Smuggling Hardening
// ============================================================================
struct ParsedHttpRequest {
    bool valid{false};
    int status_code{200};
    std::string method;
    std::string path;
    std::string http_version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::string error;

    enum class TransferEncoding { Identity, Chunked };
    TransferEncoding transfer_encoding{TransferEncoding::Identity};
    int64_t content_length{-1};
    size_t bytes_consumed{0}; // PKG-127: Track exact consumed bytes for pipelining
};

// UTF-8 Validation Helper (RFC 3629 / RFC 6455 §8.1)
inline bool is_valid_utf8(const std::vector<uint8_t>& bytes) {
    size_t i = 0;
    while (i < bytes.size()) {
        uint8_t c = bytes[i];
        if (c <= 0x7F) {
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= bytes.size() || (bytes[i + 1] & 0xC0) != 0x80) return false;
            if (c < 0xC2) return false; // Overlong encoding
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= bytes.size() || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80) return false;
            if (c == 0xE0 && (bytes[i + 1] < 0xA0)) return false; // Overlong
            if (c == 0xED && (bytes[i + 1] >= 0xA0)) return false; // Surrogate halves
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= bytes.size() || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80) return false;
            if (c == 0xF0 && (bytes[i + 1] < 0x90)) return false; // Overlong
            if (c > 0xF4 || (c == 0xF4 && bytes[i + 1] > 0x8F)) return false; // Above U+10FFFF
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

inline ParsedHttpRequest parse_http_request(std::string_view raw) {
    constexpr size_t kMaxBodySize = 10 * 1024 * 1024; // 10 MB limit
    ParsedHttpRequest result;
    std::string data(raw);

    // PKG-126: Find end of headers
    auto header_end = data.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        result.error = "Incomplete HTTP request: missing header terminator";
        result.status_code = 400;
        return result;
    }

    // PKG-126: Request Header Fields Too Large check (RFC 6585)
    constexpr size_t kMaxHeaderSectionSize = 8192;
    if (header_end > kMaxHeaderSectionSize) {
        result.error = "Request Header Fields Too Large: header section exceeds 8192 bytes";
        result.status_code = 431;
        return result;
    }

    std::string header_section = data.substr(0, header_end);
    std::string body_section = data.substr(header_end + 4);

    // Parse request line
    auto first_line_end = header_section.find("\r\n");
    if (first_line_end == std::string::npos) {
        first_line_end = header_section.size();
    }
    if (first_line_end > 4096) {
        result.error = "URI Too Long: request line exceeds 4096 bytes";
        result.status_code = 414;
        return result;
    }

    std::string request_line = header_section.substr(0, first_line_end);

    // Split request line: METHOD PATH VERSION
    auto sp1 = request_line.find(' ');
    auto sp2 = request_line.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) {
        result.error = "Malformed request line: " + request_line;
        result.status_code = 400;
        return result;
    }

    result.method = request_line.substr(0, sp1);
    result.path = request_line.substr(sp1 + 1, sp2 - sp1 - 1);
    result.http_version = request_line.substr(sp2 + 1);

    // Validate method
    static const std::vector<std::string> valid_methods = {
        "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"
    };
    bool method_valid = false;
    for (const auto& m : valid_methods) {
        if (result.method == m) { method_valid = true; break; }
    }
    if (!method_valid) {
        result.error = "Invalid HTTP method: " + result.method;
        result.status_code = 400;
        return result;
    }

    // PKG-126: Parse and validate headers with strict smuggling prevention
    size_t pos = first_line_end + 2;
    bool has_transfer_encoding = false;
    bool has_content_length = false;
    int content_length_count = 0;
    std::string observed_cl_str;

    while (pos < header_section.size()) {
        auto line_end = header_section.find("\r\n", pos);
        if (line_end == std::string::npos) line_end = header_section.size();

        std::string line = header_section.substr(pos, line_end - pos);
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            // RFC 7230 §3.2.4: No whitespace allowed between header name and colon
            if (colon > 0 && (line[colon - 1] == ' ' || line[colon - 1] == '\t')) {
                result.error = "RFC 7230 violation: invalid whitespace before colon in header name";
                result.status_code = 400;
                return result;
            }

            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            auto vstart = value.find_first_not_of(" \t");
            if (vstart != std::string::npos) {
                value = value.substr(vstart);
            }
            auto vend = value.find_last_not_of(" \t");
            if (vend != std::string::npos) {
                value = value.substr(0, vend + 1);
            }

            // Case-insensitive checks for framing headers
            std::string key_lower = key;
            std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            if (key_lower == "transfer-encoding") {
                has_transfer_encoding = true;
                if (value.find("chunked") != std::string::npos) {
                    result.transfer_encoding = ParsedHttpRequest::TransferEncoding::Chunked;
                }
            } else if (key_lower == "content-length") {
                has_content_length = true;
                content_length_count++;
                if (content_length_count > 1) {
                    if (value != observed_cl_str) {
                        // RFC 9112 §6.1: Multiple different Content-Length values -> MUST reject!
                        result.error = "RFC 9112 request smuggling violation: conflicting duplicate Content-Length headers";
                        result.status_code = 400;
                        return result;
                    }
                    // RFC 9112 §6.1: Duplicate identical Content-Length values -> collapse to single value
                }
                observed_cl_str = value;
            }

            result.headers[key] = value;
        }
        pos = line_end + 2;
    }

    // PKG-126/127: RFC 9112 §6.1 & RFC 7230 §3.3.3 Request Smuggling Check:
    // "If a message is received with both a Transfer-Encoding and a Content-Length header field,
    // the Content-Length MUST be rejected or removed; servers SHOULD treat this as an invalid request."
    if (has_transfer_encoding && has_content_length) {
        result.error = "RFC 7230 request smuggling violation: both Transfer-Encoding and Content-Length present";
        result.status_code = 400;
        return result;
    }

    // Parse Content-Length value if present with strict integer overflow & non-decimal validation
    if (has_content_length) {
        if (observed_cl_str.empty()) {
            result.error = "Empty Content-Length header";
            result.status_code = 400;
            return result;
        }
        for (char c : observed_cl_str) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                result.error = "Malformed non-numeric Content-Length: " + observed_cl_str;
                result.status_code = 400;
                return result;
            }
        }
        try {
            unsigned long long val = std::stoull(observed_cl_str);
            if (val > static_cast<unsigned long long>(kMaxBodySize)) {
                result.error = "Payload Too Large: Content-Length exceeds 10 MB limit";
                result.status_code = 413;
                return result;
            }
            result.content_length = static_cast<int64_t>(val);
        } catch (const std::out_of_range&) {
            result.error = "Content-Length integer overflow: " + observed_cl_str;
            result.status_code = 400;
            return result;
        } catch (...) {
            result.error = "Malformed non-numeric Content-Length: " + observed_cl_str;
            result.status_code = 400;
            return result;
        }
    }

    // PKG-126/127: Parse body with hardened chunk decoding, extensions, trailers and size limits
    if (result.transfer_encoding == ParsedHttpRequest::TransferEncoding::Chunked) {
        std::string decoded_body;
        size_t chunk_pos = 0;

        while (chunk_pos < body_section.size()) {
            auto chunk_size_end = body_section.find("\r\n", chunk_pos);
            if (chunk_size_end == std::string::npos) {
                result.error = "Malformed chunked encoding: missing CRLF after chunk size";
                result.status_code = 400;
                return result;
            }

            std::string chunk_size_raw = body_section.substr(chunk_pos, chunk_size_end - chunk_pos);
            auto semi = chunk_size_raw.find(';');
            std::string chunk_size_hex = (semi != std::string::npos) ? chunk_size_raw.substr(0, semi) : chunk_size_raw;

            // RFC 9112 §7.1.1: Validate chunk extensions
            if (semi != std::string::npos) {
                std::string ext = chunk_size_raw.substr(semi + 1);
                for (char c : ext) {
                    if (static_cast<unsigned char>(c) <= 31 && c != '\t') {
                        result.error = "Malformed chunk extension: illegal control character";
                        result.status_code = 400;
                        return result;
                    }
                }
            }

            // Trim spaces
            auto hstart = chunk_size_hex.find_first_not_of(" \t");
            auto hend = chunk_size_hex.find_last_not_of(" \t");
            if (hstart == std::string::npos) {
                result.error = "Malformed chunked encoding: empty chunk size";
                result.status_code = 400;
                return result;
            }
            chunk_size_hex = chunk_size_hex.substr(hstart, hend - hstart + 1);

            // Validate that chunk size contains ONLY valid hex digits
            for (char c : chunk_size_hex) {
                if (!std::isxdigit(static_cast<unsigned char>(c))) {
                    result.error = "Malformed chunked encoding: non-hex character in chunk size: " + chunk_size_hex;
                    result.status_code = 400;
                    return result;
                }
            }

            size_t chunk_size = 0;
            try {
                chunk_size = std::stoull(chunk_size_hex, nullptr, 16);
            } catch (...) {
                result.error = "Malformed chunked encoding: chunk size overflow";
                result.status_code = 400;
                return result;
            }

            if (chunk_size == 0) {
                // Final chunk 0\r\n. Check for chunk trailers per RFC 9112 §7.1.2
                size_t trailer_pos = chunk_size_end + 2;
                if (body_section.size() >= trailer_pos + 2 &&
                    body_section[trailer_pos] == '\r' && body_section[trailer_pos + 1] == '\n') {
                    // Empty trailer section (terminating CRLF)
                    chunk_pos = trailer_pos + 2;
                } else {
                    auto trailer_end = body_section.find("\r\n\r\n", trailer_pos);
                    if (trailer_end != std::string::npos) {
                        std::string trailer_data = body_section.substr(trailer_pos, trailer_end - trailer_pos);
                        if (trailer_data.find("Transfer-Encoding") != std::string::npos ||
                            trailer_data.find("Content-Length") != std::string::npos) {
                            result.error = "RFC 9112 violation: forbidden trailer field in chunked encoding";
                            result.status_code = 400;
                            return result;
                        }
                        chunk_pos = trailer_end + 4;
                    } else {
                        chunk_pos = trailer_pos;
                    }
                }
                break;
            }

            size_t chunk_data_start = chunk_size_end + 2;
            if (chunk_data_start + chunk_size > body_section.size()) {
                result.error = "Truncated chunked body data";
                result.status_code = 400;
                return result;
            }

            // Verify chunk data is strictly terminated by CRLF
            if (body_section.size() < chunk_data_start + chunk_size + 2 ||
                body_section[chunk_data_start + chunk_size] != '\r' ||
                body_section[chunk_data_start + chunk_size + 1] != '\n') {
                result.error = "Malformed chunked encoding: chunk data not followed by CRLF";
                result.status_code = 400;
                return result;
            }

            decoded_body += body_section.substr(chunk_data_start, chunk_size);
            if (decoded_body.size() > kMaxBodySize) {
                result.error = "Payload Too Large: chunked body exceeds 10 MB limit";
                result.status_code = 413;
                return result;
            }

            chunk_pos = chunk_data_start + chunk_size + 2;
        }
        result.body = std::move(decoded_body);
        result.bytes_consumed = header_end + 4 + chunk_pos;
    } else if (result.content_length > 0) {
        if (static_cast<size_t>(result.content_length) > kMaxBodySize) {
            result.error = "Payload Too Large: Content-Length exceeds 10 MB limit";
            result.status_code = 413;
            return result;
        }
        if (static_cast<int64_t>(body_section.size()) < result.content_length) {
            result.error = "Body shorter than Content-Length";
            result.status_code = 400;
            return result;
        }
        result.body = body_section.substr(0, static_cast<size_t>(result.content_length));
        result.bytes_consumed = header_end + 4 + static_cast<size_t>(result.content_length);
    } else {
        // RFC 9112 §6.3: No Content-Length or Transfer-Encoding -> 0 body bytes
        result.body.clear();
        result.bytes_consumed = header_end + 4;
    }

    result.valid = true;
    return result;
}

// ============================================================================
// PKG-127: Pipelined HTTP/1.1 Request Stream Parser
// ============================================================================
struct PipelinedParseResult {
    std::vector<ParsedHttpRequest> requests;
    size_t total_bytes_consumed{0};
    bool incomplete{false};
    std::string error;
};

inline PipelinedParseResult parse_http_requests_pipelined(std::string_view stream_buffer) {
    PipelinedParseResult result;
    size_t offset = 0;

    while (offset < stream_buffer.size()) {
        std::string_view remaining = stream_buffer.substr(offset);
        if (remaining.empty()) break;

        auto header_end = remaining.find("\r\n\r\n");
        if (header_end == std::string_view::npos) {
            result.incomplete = true;
            break;
        }

        auto parsed = parse_http_request(remaining);
        if (!parsed.valid) {
            if (parsed.error.find("Incomplete") != std::string::npos ||
                parsed.error.find("Truncated") != std::string::npos ||
                parsed.error.find("Body shorter") != std::string::npos) {
                result.incomplete = true;
                break;
            }
            result.error = parsed.error;
            result.requests.push_back(parsed);
            break;
        }

        if (parsed.bytes_consumed == 0) {
            parsed.bytes_consumed = header_end + 4;
        }

        offset += parsed.bytes_consumed;
        result.requests.push_back(std::move(parsed));
    }

    result.total_bytes_consumed = offset;
    return result;
}

// ============================================================================
// PKG-127: TLS 1.2 / 1.3 Record Layer & ClientHello Protocol Inspector
// ============================================================================
struct TlsRecordHeader {
    enum class ContentType : uint8_t {
        ChangeCipherSpec = 20,
        Alert = 21,
        Handshake = 22,
        ApplicationData = 23,
        Heartbeat = 24
    };

    ContentType type{ContentType::Handshake};
    uint16_t legacy_version{0x0303}; // TLS 1.2 (0x0303)
    uint16_t length{0};
    bool valid{false};
    std::string error;
};

struct TlsClientHelloInfo {
    bool valid{false};
    uint16_t client_version{0};
    std::string server_name; // SNI hostname
    std::vector<std::string> alpn_protocols; // Negotiated ALPN (e.g. "h2", "http/1.1")
    std::vector<uint16_t> cipher_suites;
    std::string error;
};

inline TlsRecordHeader parse_tls_record_header(const uint8_t* data, size_t size) {
    TlsRecordHeader header;
    if (size < 5) {
        header.error = "Incomplete TLS record: header requires at least 5 bytes";
        return header;
    }

    uint8_t ct = data[0];
    if (ct < 20 || ct > 24) {
        header.error = "Invalid TLS ContentType: " + std::to_string(ct);
        return header;
    }
    header.type = static_cast<TlsRecordHeader::ContentType>(ct);
    header.legacy_version = static_cast<uint16_t>((static_cast<uint16_t>(data[1]) << 8) | static_cast<uint16_t>(data[2]));
    header.length = static_cast<uint16_t>((static_cast<uint16_t>(data[3]) << 8) | static_cast<uint16_t>(data[4]));

    if (header.length > 18432) {
        header.error = "Oversized TLS record length: " + std::to_string(header.length);
        return header;
    }

    header.valid = true;
    return header;
}

inline TlsClientHelloInfo parse_tls_client_hello(const uint8_t* data, size_t size) {
    TlsClientHelloInfo info;
    auto record = parse_tls_record_header(data, size);
    if (!record.valid) {
        info.error = record.error;
        return info;
    }

    if (record.type != TlsRecordHeader::ContentType::Handshake) {
        info.error = "Not a TLS Handshake record";
        return info;
    }

    if (size < 5 + record.length) {
        info.error = "Truncated TLS handshake record payload";
        return info;
    }

    size_t offset = 5;
    uint8_t handshake_type = data[offset];
    if (handshake_type != 1) { // 1 = ClientHello
        info.error = "Handshake is not ClientHello (type=" + std::to_string(handshake_type) + ")";
        return info;
    }

    uint32_t handshake_len = (static_cast<uint32_t>(data[offset + 1]) << 16) |
                             (static_cast<uint32_t>(data[offset + 2]) << 8) |
                             static_cast<uint32_t>(data[offset + 3]);
    offset += 4;

    if (offset + handshake_len > 5 + record.length) {
        info.error = "Malformed ClientHello: length exceeds record payload";
        return info;
    }

    if (offset + 34 > size) {
        info.error = "Truncated ClientHello: missing client version and random";
        return info;
    }

    info.client_version = static_cast<uint16_t>((static_cast<uint16_t>(data[offset]) << 8) | static_cast<uint16_t>(data[offset + 1]));
    offset += 2; // version
    offset += 32; // client random

    // Session ID
    if (offset >= size) { info.error = "Truncated session ID length"; return info; }
    uint8_t session_id_len = data[offset++];
    if (offset + session_id_len > size) { info.error = "Truncated session ID"; return info; }
    offset += session_id_len;

    // Cipher Suites
    if (offset + 2 > size) { info.error = "Truncated cipher suites length"; return info; }
    uint16_t cipher_suites_len = static_cast<uint16_t>((static_cast<uint16_t>(data[offset]) << 8) | static_cast<uint16_t>(data[offset + 1]));
    offset += 2;
    if (offset + cipher_suites_len > size) { info.error = "Truncated cipher suites"; return info; }

    for (size_t c = 0; c + 1 < cipher_suites_len; c += 2) {
        uint16_t cs = static_cast<uint16_t>((static_cast<uint16_t>(data[offset + c]) << 8) | static_cast<uint16_t>(data[offset + c + 1]));
        info.cipher_suites.push_back(cs);
    }
    offset += cipher_suites_len;

    // Compression Methods
    if (offset >= size) { info.error = "Truncated compression methods length"; return info; }
    uint8_t comp_len = data[offset++];
    offset += comp_len;

    // Extensions
    if (offset + 2 <= size) {
        uint16_t ext_total_len = static_cast<uint16_t>((static_cast<uint16_t>(data[offset]) << 8) | static_cast<uint16_t>(data[offset + 1]));
        offset += 2;
        size_t ext_end = offset + ext_total_len;
        if (ext_end <= size) {
            while (offset + 4 <= ext_end) {
                uint16_t ext_type = static_cast<uint16_t>((static_cast<uint16_t>(data[offset]) << 8) | static_cast<uint16_t>(data[offset + 1]));
                uint16_t ext_len = static_cast<uint16_t>((static_cast<uint16_t>(data[offset + 2]) << 8) | static_cast<uint16_t>(data[offset + 3]));
                offset += 4;

                if (offset + ext_len > ext_end) break;

                if (ext_type == 0x0000) { // Server Name Indication (SNI)
                    if (ext_len >= 5) {
                        uint16_t name_len = static_cast<uint16_t>((static_cast<uint16_t>(data[offset + 3]) << 8) | static_cast<uint16_t>(data[offset + 4]));
                        if (offset + 5 + name_len <= ext_end) {
                            info.server_name = std::string(reinterpret_cast<const char*>(&data[offset + 5]), name_len);
                        }
                    }
                } else if (ext_type == 0x0010) { // ALPN (RFC 7301)
                    if (ext_len >= 2) {
                        uint16_t alpn_list_len = static_cast<uint16_t>((static_cast<uint16_t>(data[offset]) << 8) | static_cast<uint16_t>(data[offset + 1]));
                        size_t a_offset = offset + 2;
                        size_t a_end = a_offset + alpn_list_len;
                        while (a_offset < a_end && a_offset < ext_end) {
                            uint8_t proto_len = data[a_offset++];
                            if (a_offset + proto_len <= a_end) {
                                info.alpn_protocols.emplace_back(reinterpret_cast<const char*>(&data[a_offset]), proto_len);
                                a_offset += proto_len;
                            } else {
                                break;
                            }
                        }
                    }
                }
                offset += ext_len;
            }
        }
    }

    info.valid = true;
    return info;
}

// Convert a ParsedHttpRequest into a Request object for routing
inline Request to_request(const ParsedHttpRequest& parsed) {
    Request req(parsed.method, parsed.path, parsed.body);
    for (const auto& [k, v] : parsed.headers) {
        req.set_header(k, v);
    }
    return req;
}

// ============================================================================
// PKG-125/126: WebSocket Frame Parser & Fragmentation Reassembly
// ============================================================================
struct WebSocketFrame {
    enum class Opcode : uint8_t {
        Continuation = 0x0,
        Text = 0x1,
        Binary = 0x2,
        Close = 0x8,
        Ping = 0x9,
        Pong = 0xA
    };

    bool fin{true};
    Opcode opcode{Opcode::Text};
    bool masked{false};
    uint8_t mask_key[4]{};
    std::vector<uint8_t> payload;

    // Encode frame to wire format
    [[nodiscard]] std::vector<uint8_t> encode() const {
        std::vector<uint8_t> frame;

        // Byte 1: FIN + opcode
        uint8_t byte1 = static_cast<uint8_t>(opcode);
        if (fin) byte1 = static_cast<uint8_t>(byte1 | 0x80u);
        frame.push_back(byte1);

        // Byte 2: MASK + payload length
        uint8_t byte2 = masked ? static_cast<uint8_t>(0x80u) : static_cast<uint8_t>(0x00u);
        if (payload.size() <= 125) {
            byte2 = static_cast<uint8_t>(byte2 | static_cast<uint8_t>(payload.size()));
            frame.push_back(byte2);
        } else if (payload.size() <= 65535) {
            byte2 = static_cast<uint8_t>(byte2 | 126u);
            frame.push_back(byte2);
            frame.push_back(static_cast<uint8_t>((payload.size() >> 8) & 0xFF));
            frame.push_back(static_cast<uint8_t>(payload.size() & 0xFF));
        } else {
            byte2 = static_cast<uint8_t>(byte2 | 127u);
            frame.push_back(byte2);
            for (int i = 7; i >= 0; --i) {
                frame.push_back(static_cast<uint8_t>((payload.size() >> (i * 8)) & 0xFF));
            }
        }

        if (masked) {
            frame.insert(frame.end(), mask_key, mask_key + 4);
        }

        if (masked) {
            for (size_t i = 0; i < payload.size(); ++i) {
                frame.push_back(static_cast<uint8_t>(payload[i] ^ mask_key[i % 4]));
            }
        } else {
            frame.insert(frame.end(), payload.begin(), payload.end());
        }

        return frame;
    }

    struct DecodeResult;
    static DecodeResult decode(const std::vector<uint8_t>& data, bool require_masked = false);

    static WebSocketFrame text(std::string_view message) {
        WebSocketFrame f;
        f.opcode = Opcode::Text;
        f.fin = true;
        f.payload.assign(message.begin(), message.end());
        return f;
    }

    static WebSocketFrame ping(std::string_view payload = "") {
        WebSocketFrame f;
        f.opcode = Opcode::Ping;
        f.fin = true;
        f.payload.assign(payload.begin(), payload.end());
        return f;
    }

    static WebSocketFrame pong(const WebSocketFrame& ping_frame) {
        WebSocketFrame f;
        f.opcode = Opcode::Pong;
        f.fin = true;
        f.payload = ping_frame.payload;
        return f;
    }

    static WebSocketFrame close(uint16_t code = 1000, std::string_view reason = "") {
        WebSocketFrame f;
        f.opcode = Opcode::Close;
        f.fin = true;
        f.payload.push_back(static_cast<uint8_t>((code >> 8) & 0xFF));
        f.payload.push_back(static_cast<uint8_t>(code & 0xFF));
        f.payload.insert(f.payload.end(), reason.begin(), reason.end());
        return f;
    }

    [[nodiscard]] std::string payload_string() const {
        return std::string(payload.begin(), payload.end());
    }

    [[nodiscard]] uint16_t close_code() const {
        if (opcode != Opcode::Close || payload.size() < 2) return 0;
        return static_cast<uint16_t>((static_cast<uint16_t>(payload[0]) << 8) | static_cast<uint16_t>(payload[1]));
    }

    [[nodiscard]] std::string close_reason() const {
        if (opcode != Opcode::Close || payload.size() <= 2) return "";
        return std::string(payload.begin() + 2, payload.end());
    }
};

struct WebSocketFrame::DecodeResult {
    bool valid{false};
    uint16_t close_error_code{1000};
    WebSocketFrame frame;
    size_t bytes_consumed{0};
    std::string error;
};

inline WebSocketFrame::DecodeResult WebSocketFrame::decode(const std::vector<uint8_t>& data, bool require_masked) {
    DecodeResult result;

    if (data.size() < 2) {
        result.error = "Frame too short";
        result.close_error_code = 1002;
        return result;
    }

    size_t pos = 0;
    result.frame.fin = (data[0] & 0x80) != 0;
    result.frame.opcode = static_cast<Opcode>(data[0] & 0x0F);
    pos++;

    result.frame.masked = (data[1] & 0x80) != 0;
    uint64_t payload_len = data[1] & 0x7F;
    pos++;

    // PKG-126: RFC 6455 §5.1: Client-to-server frames MUST be masked
    if (require_masked && !result.frame.masked) {
        result.error = "RFC 6455 violation: client frames must be masked";
        result.close_error_code = 1002; // Protocol Error
        return result;
    }

    // PKG-126: RFC 6455 §5.5: Control frames (Ping, Pong, Close) must not be fragmented and length <= 125
    bool is_control = (result.frame.opcode == Opcode::Ping ||
                       result.frame.opcode == Opcode::Pong ||
                       result.frame.opcode == Opcode::Close);
    if (is_control) {
        if (!result.frame.fin) {
            result.error = "RFC 6455 violation: control frames must not be fragmented";
            result.close_error_code = 1002;
            return result;
        }
        if (payload_len > 125) {
            result.error = "RFC 6455 violation: control frame payload length exceeds 125 bytes";
            result.close_error_code = 1002;
            return result;
        }
    }

    if (payload_len == 126) {
        if (data.size() < pos + 2) {
            result.error = "Incomplete extended length";
            return result;
        }
        payload_len = (static_cast<uint64_t>(data[pos]) << 8) | static_cast<uint64_t>(data[pos + 1]);
        pos += 2;
    } else if (payload_len == 127) {
        if (data.size() < pos + 8) {
            result.error = "Incomplete 64-bit length";
            return result;
        }
        payload_len = 0;
        for (size_t i = 0; i < 8; ++i) {
            payload_len = (payload_len << 8) | static_cast<uint64_t>(data[pos + i]);
        }
        pos += 8;
    }

    if (result.frame.masked) {
        if (data.size() < pos + 4) {
            result.error = "Incomplete mask key";
            return result;
        }
        std::memcpy(result.frame.mask_key, &data[pos], 4);
        pos += 4;
    }

    if (data.size() < pos + payload_len) {
        result.error = "Incomplete payload";
        return result;
    }

    result.frame.payload.resize(payload_len);
    std::memcpy(result.frame.payload.data(), &data[pos], payload_len);

    if (result.frame.masked) {
        for (size_t i = 0; i < payload_len; ++i) {
            result.frame.payload[i] = static_cast<uint8_t>(result.frame.payload[i] ^ result.frame.mask_key[i % 4]);
        }
    }

    // PKG-126: RFC 6455 §8.1: Validate UTF-8 for Text frames
    if (result.frame.opcode == Opcode::Text) {
        if (!is_valid_utf8(result.frame.payload)) {
            result.error = "RFC 6455 violation: invalid UTF-8 sequence in text frame";
            result.close_error_code = 1007; // Invalid Frame Payload Data
            return result;
        }
    }

    result.bytes_consumed = pos + payload_len;
    result.valid = true;
    return result;
}

// PKG-126: WebSocket Message Assembler for handling fragmented frames
class WebSocketMessageAssembler {
public:
    struct ReassemblyResult {
        bool complete{false};
        WebSocketFrame::Opcode opcode{WebSocketFrame::Opcode::Text};
        std::vector<uint8_t> payload;
        std::string error;
    };

    ReassemblyResult process_frame(const WebSocketFrame& frame) {
        ReassemblyResult result;

        if (frame.opcode != WebSocketFrame::Opcode::Continuation) {
            // New message sequence
            current_opcode_ = frame.opcode;
            buffer_.clear();
            buffer_.insert(buffer_.end(), frame.payload.begin(), frame.payload.end());

            if (frame.fin) {
                // Single unfragmented frame
                result.complete = true;
                result.opcode = current_opcode_;
                result.payload = std::move(buffer_);
                buffer_.clear();
            }
        } else {
            // Continuation frame
            buffer_.insert(buffer_.end(), frame.payload.begin(), frame.payload.end());
            if (frame.fin) {
                // Final continuation frame
                result.complete = true;
                result.opcode = current_opcode_;
                result.payload = std::move(buffer_);
                buffer_.clear();
            }
        }

        // Validate UTF-8 on reassembled text messages
        if (result.complete && result.opcode == WebSocketFrame::Opcode::Text) {
            if (!is_valid_utf8(result.payload)) {
                result.complete = false;
                result.error = "Invalid UTF-8 in reassembled fragmented text message";
            }
        }

        return result;
    }

    void reset() {
        buffer_.clear();
        current_opcode_ = WebSocketFrame::Opcode::Text;
    }

private:
    WebSocketFrame::Opcode current_opcode_{WebSocketFrame::Opcode::Text};
    std::vector<uint8_t> buffer_;
};

// ============================================================================
// PKG-125: Connection Backpressure & Tracking
// ============================================================================
struct BackpressureConfig {
    size_t high_watermark{65536};
    size_t low_watermark{16384};
    std::chrono::seconds slow_client_timeout{30};
};

class ConnectionTracker {
public:
    struct ConnectionInfo {
        uint64_t id{0};
        std::string remote_addr;
        std::chrono::steady_clock::time_point connected_at;
        size_t bytes_sent{0};
        size_t bytes_received{0};
        size_t write_buffer_size{0};
        bool read_paused{false};
        bool is_websocket{false};
    };

    uint64_t add_connection(const std::string& remote_addr, bool is_ws = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t id = ++next_id_;
        connections_[id] = ConnectionInfo{
            id, remote_addr,
            std::chrono::steady_clock::now(),
            0, 0, 0, false, is_ws
        };
        return id;
    }

    void remove_connection(uint64_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.erase(id);
    }

    bool update_write_buffer(uint64_t id, size_t buffer_size, const BackpressureConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = connections_.find(id);
        if (it == connections_.end()) return false;

        it->second.write_buffer_size = buffer_size;

        if (buffer_size >= config.high_watermark && !it->second.read_paused) {
            it->second.read_paused = true;
            return true;
        }
        if (buffer_size <= config.low_watermark && it->second.read_paused) {
            it->second.read_paused = false;
        }
        return false;
    }

    void record_bytes_sent(uint64_t id, size_t bytes) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = connections_.find(id);
        if (it != connections_.end()) it->second.bytes_sent += bytes;
    }

    void record_bytes_received(uint64_t id, size_t bytes) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = connections_.find(id);
        if (it != connections_.end()) it->second.bytes_received += bytes;
    }

    [[nodiscard]] std::optional<ConnectionInfo> get_connection(uint64_t id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = connections_.find(id);
        if (it != connections_.end()) return it->second;
        return std::nullopt;
    }

    [[nodiscard]] size_t active_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return connections_.size();
    }

    [[nodiscard]] std::vector<uint64_t> find_slow_clients(const BackpressureConfig& config) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<uint64_t> slow;
        auto now = std::chrono::steady_clock::now();
        for (const auto& [id, info] : connections_) {
            if (info.read_paused) {
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - info.connected_at);
                if (duration > config.slow_client_timeout) {
                    slow.push_back(id);
                }
            }
        }
        return slow;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<uint64_t, ConnectionInfo> connections_;
    uint64_t next_id_{0};
};

// ============================================================================
// WebSocket Broadcaster (unchanged from PKG-124)
// ============================================================================
class WebSocketBroadcaster {
public:
    static WebSocketBroadcaster& instance() {
        static WebSocketBroadcaster s_instance;
        return s_instance;
    }

    void subscribe(const std::string& topic, std::function<void(const std::string&)> client_cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        subscribers_[topic].push_back(std::move(client_cb));
    }

    void broadcast(const std::string& topic, const std::string& payload) {
        std::vector<std::function<void(const std::string&)>> copy;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscribers_.find(topic);
            if (it != subscribers_.end()) {
                copy = it->second;
            }
        }
        for (const auto& cb : copy) {
            if (cb) cb(payload);
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        subscribers_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<std::function<void(const std::string&)>>> subscribers_;
};

// ============================================================================
// Server (extended with protocol-level handling)
// ============================================================================
class Server {
public:
    Server() = default;
    ~Server() { stop(); }

    Server& use(Middleware mw) {
        middlewares_.push_back(std::move(mw));
        return *this;
    }

    Server& get(std::string path, RouteHandler handler) {
        routes_.push_back(RouteEntry{"GET", std::move(path), std::move(handler)});
        return *this;
    }

    Server& post(std::string path, RouteHandler handler) {
        routes_.push_back(RouteEntry{"POST", std::move(path), std::move(handler)});
        return *this;
    }

    Server& put(std::string path, RouteHandler handler) {
        routes_.push_back(RouteEntry{"PUT", std::move(path), std::move(handler)});
        return *this;
    }

    Server& delete_(std::string path, RouteHandler handler) {
        routes_.push_back(RouteEntry{"DELETE", std::move(path), std::move(handler)});
        return *this;
    }

    /// PKG-126: Handle raw HTTP bytes with status code propagation (400, 413, 414, 431)
    [[nodiscard]] Response handle_raw_request(std::string_view raw_bytes) const {
        auto parsed = parse_http_request(raw_bytes);
        if (!parsed.valid) {
            Response res;
            res.status(parsed.status_code > 0 ? parsed.status_code : 400).send(parsed.error);
            return res;
        }
        Request req = to_request(parsed);
        return handle_request(std::move(req));
    }

    // PKG-130: Server Request Logging for Audit & Network Verification
    struct RequestLogEntry {
        std::string method;
        std::string path;
        int status_code{0};
        size_t bytes_sent{0};
        std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    };

    void clear_request_logs() const {
        std::lock_guard<std::mutex> lock(log_mutex_);
        request_logs_.clear();
    }

    [[nodiscard]] std::vector<RequestLogEntry> request_logs() const {
        std::lock_guard<std::mutex> lock(log_mutex_);
        return request_logs_;
    }

    /// Dispatches a request through the middleware pipeline and routing table
    [[nodiscard]] Response handle_request(Request req) const {
        std::string method = req.method();
        std::string path = req.path();
        Response res;
        execute_pipeline(req, res, 0);

        std::lock_guard<std::mutex> lock(log_mutex_);
        request_logs_.push_back(RequestLogEntry{
            method,
            path,
            res.status_code(),
            res.body().size(),
            std::chrono::system_clock::now()
        });
        return res;
    }

    /// Listens on the specified port
    pingdart::core::Status listen(uint16_t port, bool non_blocking = false);

    // PKG-127: Production TCP Server Configuration
    struct ServerConfig {
        size_t max_connections{1024};
        std::chrono::milliseconds client_header_timeout{5000};
        std::chrono::milliseconds client_body_timeout{10000};
        std::chrono::milliseconds keep_alive_timeout{15000};
    };

    void set_server_config(ServerConfig config) { server_config_ = std::move(config); }
    [[nodiscard]] const ServerConfig& server_config() const noexcept { return server_config_; }

    [[nodiscard]] bool can_accept_connection() const noexcept {
        return tracker_.active_count() < server_config_.max_connections;
    }

    void drain(std::chrono::milliseconds timeout = std::chrono::milliseconds(500)) {
        is_draining_.store(true);
        auto start = std::chrono::steady_clock::now();
        while (tracker_.active_count() > 0) {
            if (std::chrono::steady_clock::now() - start > timeout) break;
            std::this_thread::yield();
        }
    }

    /// Stops server listener
    void stop();

    [[nodiscard]] bool is_running() const noexcept { return is_running_.load(); }
    [[nodiscard]] uint16_t port() const noexcept { return port_; }

    ConnectionTracker& connections() { return tracker_; }
    const ConnectionTracker& connections() const { return tracker_; }

    void set_backpressure_config(const BackpressureConfig& config) {
        bp_config_ = config;
    }

private:
    struct RouteEntry {
        std::string method;
        std::string path;
        RouteHandler handler;
    };

    void execute_pipeline(Request& req, Response& res, size_t mw_index) const {
        if (mw_index < middlewares_.size()) {
            middlewares_[mw_index](req, res, [&]() {
                execute_pipeline(req, res, mw_index + 1);
            });
            return;
        }

        for (const auto& r : routes_) {
            if (r.method == req.method() && match_path(r.path, req)) {
                r.handler(req, res);
                return;
            }
        }

        res.status(404).send("Not Found");
    }

    static bool match_path(const std::string& pattern, Request& req) {
        if (pattern == req.path()) return true;

        auto pat_parts = split(pattern, '/');
        auto req_parts = split(req.path(), '/');

        if (pat_parts.size() != req_parts.size()) return false;

        for (size_t i = 0; i < pat_parts.size(); ++i) {
            if (!pat_parts[i].empty() && pat_parts[i][0] == ':') {
                std::string key = pat_parts[i].substr(1);
                req.set_param(key, req_parts[i]);
            } else if (pat_parts[i] != req_parts[i]) {
                return false;
            }
        }
        return true;
    }

    static std::vector<std::string> split(std::string_view s, char delim) {
        std::vector<std::string> parts;
        size_t start = 0;
        while (start < s.size()) {
            size_t end = s.find(delim, start);
            if (end == std::string_view::npos) {
                parts.emplace_back(s.substr(start));
                break;
            }
            parts.emplace_back(s.substr(start, end - start));
            start = end + 1;
        }
        return parts;
    }

    std::vector<Middleware> middlewares_;
    std::vector<RouteEntry> routes_;
    std::atomic<bool> is_running_{false};
    std::atomic<bool> is_draining_{false};
    uint16_t port_{0};
    std::unique_ptr<std::thread> listener_thread_;
    ConnectionTracker tracker_;
    BackpressureConfig bp_config_;
    ServerConfig server_config_;
    mutable std::mutex log_mutex_;
    mutable std::vector<RequestLogEntry> request_logs_;
};

} // namespace pingcore

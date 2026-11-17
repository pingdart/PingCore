// include/pingdart/core/status.h
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace pingdart::core {

enum class StatusCode : int32_t {
    Ok = 0,
    InvalidArgument = 1,
    NotFound = 2,
    AlreadyExists = 3,
    PermissionDenied = 4,
    ResourceExhausted = 5,
    FailedPrecondition = 6,
    Aborted = 7,
    OutOfRange = 8,
    Unimplemented = 9,
    Internal = 10,
    Unavailable = 11,
    Timeout = 12,
    DataLoss = 13,
    Unauthenticated = 14
};

[[nodiscard]] constexpr std::string_view StatusCodeToString(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "Ok";
        case StatusCode::InvalidArgument: return "InvalidArgument";
        case StatusCode::NotFound: return "NotFound";
        case StatusCode::AlreadyExists: return "AlreadyExists";
        case StatusCode::PermissionDenied: return "PermissionDenied";
        case StatusCode::ResourceExhausted: return "ResourceExhausted";
        case StatusCode::FailedPrecondition: return "FailedPrecondition";
        case StatusCode::Aborted: return "Aborted";
        case StatusCode::OutOfRange: return "OutOfRange";
        case StatusCode::Unimplemented: return "Unimplemented";
        case StatusCode::Internal: return "Internal";
        case StatusCode::Unavailable: return "Unavailable";
        case StatusCode::Timeout: return "Timeout";
        case StatusCode::DataLoss: return "DataLoss";
        case StatusCode::Unauthenticated: return "Unauthenticated";
    }
    return "Unknown";
}

class [[nodiscard]] Status {
public:
    Status() noexcept : code_(StatusCode::Ok), platform_error_(0) {}
    
    explicit Status(StatusCode code, std::string message = {}, int32_t platform_error = 0)
        : code_(code), message_(std::move(message)), platform_error_(platform_error) {}

    [[nodiscard]] bool ok() const noexcept { return code_ == StatusCode::Ok; }
    [[nodiscard]] StatusCode code() const noexcept { return code_; }
    [[nodiscard]] std::string_view message() const noexcept { return message_; }
    [[nodiscard]] int32_t platform_error() const noexcept { return platform_error_; }

    [[nodiscard]] std::string to_string() const {
        if (ok()) return "Ok";
        std::string s = std::string(StatusCodeToString(code_));
        if (!message_.empty()) {
            s += ": ";
            s += message_;
        }
        if (platform_error_ != 0) {
            s += " (platform error: ";
            s += std::to_string(platform_error_);
            s += ")";
        }
        return s;
    }

    bool operator==(const Status& other) const noexcept {
        return code_ == other.code_ && message_ == other.message_ && platform_error_ == other.platform_error_;
    }

    bool operator!=(const Status& other) const noexcept {
        return !(*this == other);
    }

    // Static factories
    static Status Ok() noexcept { return Status(); }
    static Status InvalidArgument(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::InvalidArgument, std::move(message), platform_error);
    }
    static Status NotFound(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::NotFound, std::move(message), platform_error);
    }
    static Status AlreadyExists(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::AlreadyExists, std::move(message), platform_error);
    }
    static Status PermissionDenied(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::PermissionDenied, std::move(message), platform_error);
    }
    static Status ResourceExhausted(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::ResourceExhausted, std::move(message), platform_error);
    }
    static Status FailedPrecondition(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::FailedPrecondition, std::move(message), platform_error);
    }
    static Status Aborted(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::Aborted, std::move(message), platform_error);
    }
    static Status OutOfRange(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::OutOfRange, std::move(message), platform_error);
    }
    static Status Unimplemented(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::Unimplemented, std::move(message), platform_error);
    }
    static Status Internal(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::Internal, std::move(message), platform_error);
    }
    static Status Unavailable(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::Unavailable, std::move(message), platform_error);
    }
    static Status Timeout(std::string message, int32_t platform_error = 0) {
        return Status(StatusCode::Timeout, std::move(message), platform_error);
    }

private:
    StatusCode code_;
    std::string message_;
    int32_t platform_error_{0};
};

} // namespace pingdart::core

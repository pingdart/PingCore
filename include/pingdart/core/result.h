// include/pingdart/core/result.h
#pragma once
#include "status.h"
#include <variant>
#include <utility>
#include <stdexcept>
#include <type_traits>

namespace pingdart::core {

template <typename T>
class [[nodiscard]] Result {
    static_assert(!std::is_same_v<T, Status>, "Result<Status> is not permitted. Use Status directly.");
    static_assert(!std::is_reference_v<T>, "Result does not support reference types.");

public:
    using value_type = T;

    // Value constructor
    template <typename U = T,
              typename = std::enable_if_t<std::is_constructible_v<T, U&&> &&
                                         !std::is_same_v<std::decay_t<U>, Result<T>> &&
                                         !std::is_same_v<std::decay_t<U>, Status>>>
    Result(U&& value) : data_(std::in_place_type<T>, std::forward<U>(value)) {}

    // Status constructor
    Result(Status status) : data_(std::in_place_type<Status>, std::move(status)) {
        if (std::get<Status>(data_).ok()) {
            data_ = Status(StatusCode::Internal, "Result initialized with Ok status instead of a value");
        }
    }

    // In-place value constructor
    template <typename... Args>
    explicit Result(std::in_place_t, Args&&... args)
        : data_(std::in_place_type<T>, std::forward<Args>(args)...) {}

    // Copy / Move constructors and assignments
    Result(const Result&) = default;
    Result& operator=(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(Result&&) noexcept = default;

    [[nodiscard]] bool has_value() const noexcept {
        return std::holds_alternative<T>(data_);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] const T& value() const & {
        ensure_ok();
        return std::get<T>(data_);
    }

    [[nodiscard]] T& value() & {
        ensure_ok();
        return std::get<T>(data_);
    }

    [[nodiscard]] const T&& value() const && {
        ensure_ok();
        return std::move(std::get<T>(data_));
    }

    [[nodiscard]] T&& value() && {
        ensure_ok();
        return std::move(std::get<T>(data_));
    }

    [[nodiscard]] const T* operator->() const noexcept {
        return &std::get<T>(data_);
    }

    [[nodiscard]] T* operator->() noexcept {
        return &std::get<T>(data_);
    }

    [[nodiscard]] const T& operator*() const & noexcept {
        return std::get<T>(data_);
    }

    [[nodiscard]] T& operator*() & noexcept {
        return std::get<T>(data_);
    }

    [[nodiscard]] const T&& operator*() const && noexcept {
        return std::move(std::get<T>(data_));
    }

    [[nodiscard]] T&& operator*() && noexcept {
        return std::move(std::get<T>(data_));
    }

    template <typename U>
    [[nodiscard]] T value_or(U&& default_value) const & {
        if (has_value()) {
            return std::get<T>(data_);
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    template <typename U>
    [[nodiscard]] T value_or(U&& default_value) && {
        if (has_value()) {
            return std::move(std::get<T>(data_));
        }
        return static_cast<T>(std::forward<U>(default_value));
    }

    [[nodiscard]] const Status& status() const noexcept {
        if (has_value()) {
            static const Status s_ok = Status::Ok();
            return s_ok;
        }
        return std::get<Status>(data_);
    }

private:
    void ensure_ok() const {
        if (!has_value()) {
            throw std::runtime_error("Attempted to access value on non-ok Result: " + 
                                     std::string(status().message()));
        }
    }

    std::variant<T, Status> data_;
};

} // namespace pingdart::core

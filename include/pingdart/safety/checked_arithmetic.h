// include/pingdart/safety/checked_arithmetic.h
// PKG-115: Overflow-Checked Arithmetic and Math Primitives
#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <pingdart/core/result.h>

namespace pingdart::safety {

/// Safely adds two integers, returning std::nullopt on overflow/underflow
template <typename T>
[[nodiscard]] constexpr std::optional<T> checked_add(T a, T b) noexcept {
    static_assert(std::is_integral_v<T>, "checked_add requires integral types");
#if defined(__has_builtin)
#  if __has_builtin(__builtin_add_overflow)
    T result;
    if (__builtin_add_overflow(a, b, &result)) {
        return std::nullopt;
    }
    return result;
#  endif
#endif
    if constexpr (std::is_signed_v<T>) {
        if (b > 0 && a > std::numeric_limits<T>::max() - b) return std::nullopt;
        if (b < 0 && a < std::numeric_limits<T>::min() - b) return std::nullopt;
    } else {
        if (a > std::numeric_limits<T>::max() - b) return std::nullopt;
    }
    return a + b;
}

/// Safely subtracts b from a, returning std::nullopt on overflow/underflow
template <typename T>
[[nodiscard]] constexpr std::optional<T> checked_sub(T a, T b) noexcept {
    static_assert(std::is_integral_v<T>, "checked_sub requires integral types");
#if defined(__has_builtin)
#  if __has_builtin(__builtin_sub_overflow)
    T result;
    if (__builtin_sub_overflow(a, b, &result)) {
        return std::nullopt;
    }
    return result;
#  endif
#endif
    if constexpr (std::is_signed_v<T>) {
        if (b < 0 && a > std::numeric_limits<T>::max() + b) return std::nullopt;
        if (b > 0 && a < std::numeric_limits<T>::min() + b) return std::nullopt;
    } else {
        if (a < b) return std::nullopt;
    }
    return a - b;
}

/// Safely multiplies two integers, returning std::nullopt on overflow/underflow
template <typename T>
[[nodiscard]] constexpr std::optional<T> checked_mul(T a, T b) noexcept {
    static_assert(std::is_integral_v<T>, "checked_mul requires integral types");
#if defined(__has_builtin)
#  if __has_builtin(__builtin_mul_overflow)
    T result;
    if (__builtin_mul_overflow(a, b, &result)) {
        return std::nullopt;
    }
    return result;
#  endif
#endif
    if (a == 0 || b == 0) return T{0};
    if constexpr (std::is_signed_v<T>) {
        if (a > 0) {
            if (b > 0 && a > std::numeric_limits<T>::max() / b) return std::nullopt;
            if (b < 0 && b < std::numeric_limits<T>::min() / a) return std::nullopt;
        } else {
            if (b > 0 && a < std::numeric_limits<T>::min() / b) return std::nullopt;
            if (b < 0 && (a == std::numeric_limits<T>::min() || b == std::numeric_limits<T>::min() || -a > std::numeric_limits<T>::max() / -b)) return std::nullopt;
        }
    } else {
        if (a > std::numeric_limits<T>::max() / b) return std::nullopt;
    }
    return a * b;
}

/// Safely divides a by b, returning std::nullopt on division by zero or INT_MIN / -1
template <typename T>
[[nodiscard]] constexpr std::optional<T> checked_div(T a, T b) noexcept {
    static_assert(std::is_integral_v<T>, "checked_div requires integral types");
    if (b == 0) return std::nullopt;
    if constexpr (std::is_signed_v<T>) {
        if (a == std::numeric_limits<T>::min() && b == -1) return std::nullopt;
    }
    return a / b;
}

/// Strong type wrapper for checked arithmetic that throws std::overflow_error on failure
template <typename T>
class Checked {
public:
    static_assert(std::is_integral_v<T>, "Checked<T> requires integral types");

    constexpr Checked() noexcept : value_(0) {}
    constexpr Checked(T val) noexcept : value_(val) {} // NOLINT: implicit conversion intended

    [[nodiscard]] constexpr T get() const noexcept { return value_; }
    constexpr explicit operator T() const noexcept { return value_; }

    constexpr Checked operator+(Checked rhs) const {
        auto res = checked_add(value_, rhs.value_);
        if (!res.has_value()) throw std::overflow_error("Integer overflow in Checked +");
        return Checked(*res);
    }

    constexpr Checked operator-(Checked rhs) const {
        auto res = checked_sub(value_, rhs.value_);
        if (!res.has_value()) throw std::overflow_error("Integer underflow in Checked -");
        return Checked(*res);
    }

    constexpr Checked operator*(Checked rhs) const {
        auto res = checked_mul(value_, rhs.value_);
        if (!res.has_value()) throw std::overflow_error("Integer overflow in Checked *");
        return Checked(*res);
    }

    constexpr Checked operator/(Checked rhs) const {
        auto res = checked_div(value_, rhs.value_);
        if (!res.has_value()) throw std::overflow_error("Division error in Checked /");
        return Checked(*res);
    }

    constexpr Checked& operator+=(Checked rhs) { *this = *this + rhs; return *this; }
    constexpr Checked& operator-=(Checked rhs) { *this = *this - rhs; return *this; }
    constexpr Checked& operator*=(Checked rhs) { *this = *this * rhs; return *this; }
    constexpr Checked& operator/=(Checked rhs) { *this = *this / rhs; return *this; }

    constexpr bool operator==(Checked rhs) const noexcept { return value_ == rhs.value_; }
    constexpr auto operator<=>(Checked rhs) const noexcept { return value_ <=> rhs.value_; }

private:
    T value_;
};

} // namespace pingdart::safety

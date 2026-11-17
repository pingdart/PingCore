// include/pingdart/safety/non_null.h
// PKG-115: Non-Null Pointer and Smart-Pointer Wrappers
#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace pingdart::safety {

/// Non-null pointer wrapper that guarantees pointer is never nullptr
template <typename T>
class NonNull {
public:
    static_assert(std::is_pointer_v<T>, "NonNull<T> requires a raw pointer type");
    using element_type = std::remove_pointer_t<T>;

    constexpr NonNull(T ptr) : ptr_(ptr) { // NOLINT: explicit validation
        if (ptr_ == nullptr) {
            throw std::invalid_argument("NonNull pointer cannot be null");
        }
    }

    NonNull(std::nullptr_t) = delete;

    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, element_type*>>>
    constexpr NonNull(NonNull<U*> other) noexcept : ptr_(other.get()) {}

    [[nodiscard]] constexpr T get() const noexcept { return ptr_; }
    constexpr explicit operator T() const noexcept { return ptr_; }

    [[nodiscard]] constexpr element_type& operator*() const noexcept { return *ptr_; }
    [[nodiscard]] constexpr T operator->() const noexcept { return ptr_; }

    constexpr bool operator==(const NonNull& rhs) const noexcept { return ptr_ == rhs.ptr_; }
    constexpr auto operator<=>(const NonNull& rhs) const noexcept { return ptr_ <=> rhs.ptr_; }

private:
    T ptr_;
};

/// Non-null smart pointer wrapper (unique_ptr / shared_ptr)
template <typename SmartPtr>
class NonNullPtr {
public:
    using element_type = typename SmartPtr::element_type;

    explicit NonNullPtr(SmartPtr ptr) : ptr_(std::move(ptr)) {
        if (!ptr_) {
            throw std::invalid_argument("NonNullPtr cannot wrap a null smart pointer");
        }
    }

    NonNullPtr(std::nullptr_t) = delete;

    [[nodiscard]] element_type& operator*() const noexcept { return *ptr_; }
    [[nodiscard]] element_type* operator->() const noexcept { return ptr_.get(); }
    [[nodiscard]] element_type* get() const noexcept { return ptr_.get(); }

    [[nodiscard]] const SmartPtr& smart_ptr() const noexcept { return ptr_; }
    [[nodiscard]] SmartPtr release() && noexcept { return std::move(ptr_); }

private:
    SmartPtr ptr_;
};

template <typename T, typename... Args>
[[nodiscard]] NonNullPtr<std::unique_ptr<T>> make_non_null_unique(Args&&... args) {
    return NonNullPtr<std::unique_ptr<T>>(std::make_unique<T>(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
[[nodiscard]] NonNullPtr<std::shared_ptr<T>> make_non_null_shared(Args&&... args) {
    return NonNullPtr<std::shared_ptr<T>>(std::make_shared<T>(std::forward<Args>(args)...));
}

} // namespace pingdart::safety

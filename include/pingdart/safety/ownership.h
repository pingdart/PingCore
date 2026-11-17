// include/pingdart/safety/ownership.h
// PKG-115: Explicit Ownership and Lifetime Abstractions
#pragma once

#include <memory>
#include <stdexcept>
#include <utility>

namespace pingdart::safety {

/// Non-nullable unique ownership wrapper for heap-allocated resources
template <typename T>
class UniqueRef {
public:
    template <typename... Args>
    static UniqueRef<T> make(Args&&... args) {
        return UniqueRef<T>(new T(std::forward<Args>(args)...));
    }

    explicit UniqueRef(T* ptr) : ptr_(ptr) {
        if (!ptr_) throw std::invalid_argument("UniqueRef cannot be constructed with nullptr");
    }

    ~UniqueRef() = default;

    UniqueRef(const UniqueRef&) = delete;
    UniqueRef& operator=(const UniqueRef&) = delete;

    UniqueRef(UniqueRef&& other) noexcept : ptr_(std::move(other.ptr_)) {}
    UniqueRef& operator=(UniqueRef&& other) noexcept {
        ptr_ = std::move(other.ptr_);
        return *this;
    }

    [[nodiscard]] T& get() noexcept { return *ptr_; }
    [[nodiscard]] const T& get() const noexcept { return *ptr_; }

    [[nodiscard]] T& operator*() noexcept { return *ptr_; }
    [[nodiscard]] const T& operator*() const noexcept { return *ptr_; }

    [[nodiscard]] T* operator->() noexcept { return ptr_.get(); }
    [[nodiscard]] const T* operator->() const noexcept { return ptr_.get(); }

    [[nodiscard]] T* raw() const noexcept { return ptr_.get(); }

private:
    std::unique_ptr<T> ptr_;
};

/// Non-owning borrowed reference guaranteeing lifetime within a bounded scope
template <typename T>
class BorrowedRef {
public:
    constexpr explicit BorrowedRef(T& ref) noexcept : ref_(&ref) {}
    constexpr BorrowedRef(const UniqueRef<T>& unique) noexcept : ref_(unique.raw()) {}

    [[nodiscard]] constexpr T& get() const noexcept { return *ref_; }
    [[nodiscard]] constexpr T& operator*() const noexcept { return *ref_; }
    [[nodiscard]] constexpr T* operator->() const noexcept { return ref_; }

private:
    T* ref_;
};

} // namespace pingdart::safety

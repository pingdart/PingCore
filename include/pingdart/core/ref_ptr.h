// include/pingdart/core/ref_ptr.h
#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <utility>
#include <type_traits>

namespace pingdart::core {

template <typename T>
class RefCounted {
public:
    void add_ref() const noexcept {
        ref_count_.fetch_add(1, std::memory_order_relaxed);
    }

    void release() const noexcept {
        if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete static_cast<const T*>(this);
        }
    }

    [[nodiscard]] uint32_t ref_count() const noexcept {
        return ref_count_.load(std::memory_order_relaxed);
    }

protected:
    RefCounted() = default;
    virtual ~RefCounted() = default;

    // Disallow copying and moving of the internal reference count
    RefCounted(const RefCounted&) noexcept : ref_count_(0) {}
    RefCounted& operator=(const RefCounted&) noexcept { return *this; }
    RefCounted(RefCounted&&) noexcept : ref_count_(0) {}
    RefCounted& operator=(RefCounted&&) noexcept { return *this; }

private:
    mutable std::atomic<uint32_t> ref_count_{0};
};

enum class AdoptRefTag { Adopt };

template <typename T>
class RefPtr {
public:
    using element_type = T;

    constexpr RefPtr() noexcept : ptr_(nullptr) {}
    constexpr RefPtr(std::nullptr_t) noexcept : ptr_(nullptr) {}

    explicit RefPtr(T* ptr) noexcept : ptr_(ptr) {
        if (ptr_) ptr_->add_ref();
    }

    // Internal adopt constructor (does not increment ref_count)
    RefPtr(AdoptRefTag, T* ptr) noexcept : ptr_(ptr) {}

    // Copy constructor
    RefPtr(const RefPtr& other) noexcept : ptr_(other.ptr_) {
        if (ptr_) ptr_->add_ref();
    }

    // Move constructor
    RefPtr(RefPtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    // Subtype/Polymorphic copy constructor
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    RefPtr(const RefPtr<U>& other) noexcept : ptr_(other.get()) {
        if (ptr_) ptr_->add_ref();
    }

    // Subtype/Polymorphic move constructor
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    RefPtr(RefPtr<U>&& other) noexcept : ptr_(other.detach()) {}

    ~RefPtr() {
        if (ptr_) ptr_->release();
    }

    // Copy assignment
    RefPtr& operator=(const RefPtr& other) noexcept {
        if (this != &other) {
            if (other.ptr_) other.ptr_->add_ref();
            if (ptr_) ptr_->release();
            ptr_ = other.ptr_;
        }
        return *this;
    }

    // Move assignment
    RefPtr& operator=(RefPtr&& other) noexcept {
        if (this != &other) {
            if (ptr_) ptr_->release();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    // Subtype/Polymorphic copy assignment
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    RefPtr& operator=(const RefPtr<U>& other) noexcept {
        if (ptr_ != other.get()) {
            if (other.get()) other.get()->add_ref();
            if (ptr_) ptr_->release();
            ptr_ = other.get();
        }
        return *this;
    }

    // Subtype/Polymorphic move assignment
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    RefPtr& operator=(RefPtr<U>&& other) noexcept {
        if (ptr_ != other.get()) {
            if (ptr_) ptr_->release();
            ptr_ = other.detach();
        } else {
            other.reset();
        }
        return *this;
    }

    [[nodiscard]] T* get() const noexcept { return ptr_; }
    [[nodiscard]] T* operator->() const noexcept { return ptr_; }
    [[nodiscard]] T& operator*() const noexcept { return *ptr_; }
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    void reset(T* ptr = nullptr) noexcept {
        if (ptr != ptr_) {
            if (ptr) ptr->add_ref();
            if (ptr_) ptr_->release();
            ptr_ = ptr;
        }
    }

    [[nodiscard]] T* detach() noexcept {
        T* p = ptr_;
        ptr_ = nullptr;
        return p;
    }

    void swap(RefPtr& other) noexcept {
        std::swap(ptr_, other.ptr_);
    }

    bool operator==(const RefPtr& other) const noexcept { return ptr_ == other.ptr_; }
    bool operator!=(const RefPtr& other) const noexcept { return ptr_ != other.ptr_; }
    bool operator==(std::nullptr_t) const noexcept { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t) const noexcept { return ptr_ != nullptr; }
    bool operator==(const T* raw) const noexcept { return ptr_ == raw; }
    bool operator!=(const T* raw) const noexcept { return ptr_ != raw; }

private:
    template <typename U>
    friend class RefPtr;

    T* ptr_{nullptr};
};

template <typename T>
void swap(RefPtr<T>& a, RefPtr<T>& b) noexcept {
    a.swap(b);
}

template <typename T, typename... Args>
[[nodiscard]] RefPtr<T> make_ref(Args&&... args) {
    return RefPtr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
[[nodiscard]] RefPtr<T> adopt_ref(T* ptr) noexcept {
    return RefPtr<T>(AdoptRefTag::Adopt, ptr);
}

} // namespace pingdart::core

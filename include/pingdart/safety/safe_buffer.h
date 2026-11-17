// include/pingdart/safety/safe_buffer.h
// PKG-115: Safe Bounds-Checked Spans and Contiguous Buffers
#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <span>
#include <string_view>
#include <type_traits>
#include <pingdart/core/result.h>

namespace pingdart::safety {

/// Non-owning view over contiguous memory with guaranteed bounds checking
template <typename T>
class SafeSpan {
public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = pointer;
    using const_iterator = const_pointer;

    constexpr SafeSpan() noexcept : data_(nullptr), size_(0) {}

    constexpr SafeSpan(pointer ptr, size_type count) noexcept
        : data_(ptr), size_(count) {}

    constexpr SafeSpan(pointer first, pointer last) noexcept
        : data_(first), size_(static_cast<size_type>(last >= first ? last - first : 0)) {}

    template <std::size_t N>
    constexpr SafeSpan(element_type (&arr)[N]) noexcept
        : data_(arr), size_(N) {}

    template <typename Container,
              typename = std::enable_if_t<
                  !std::is_same_v<std::decay_t<Container>, SafeSpan> &&
                  std::is_convertible_v<decltype(std::declval<Container>().data()), pointer>>>
    constexpr SafeSpan(Container& c) noexcept
        : data_(c.data()), size_(c.size()) {}

    template <typename Container,
              typename = std::enable_if_t<
                  !std::is_same_v<std::decay_t<Container>, SafeSpan> &&
                  std::is_convertible_v<decltype(std::declval<const Container>().data()), pointer>>>
    constexpr SafeSpan(const Container& c) noexcept
        : data_(c.data()), size_(c.size()) {}

    [[nodiscard]] constexpr pointer data() const noexcept { return data_; }
    [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
    [[nodiscard]] constexpr size_type size_bytes() const noexcept { return size_ * sizeof(T); }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

    [[nodiscard]] constexpr reference at(size_type index) const {
        if (index >= size_) {
            throw std::out_of_range("SafeSpan::at index out of range: " + std::to_string(index) + " >= " + std::to_string(size_));
        }
        return data_[index];
    }

    [[nodiscard]] constexpr reference operator[](size_type index) const {
        return at(index);
    }

    [[nodiscard]] constexpr reference front() const {
        return at(0);
    }

    [[nodiscard]] constexpr reference back() const {
        if (size_ == 0) {
            throw std::out_of_range("SafeSpan::back called on empty span");
        }
        return data_[size_ - 1];
    }

    [[nodiscard]] constexpr iterator begin() const noexcept { return data_; }
    [[nodiscard]] constexpr iterator end() const noexcept { return data_ + size_; }
    [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data_; }
    [[nodiscard]] constexpr const_iterator cend() const noexcept { return data_ + size_; }

    [[nodiscard]] constexpr SafeSpan<T> subspan(size_type offset, size_type count = static_cast<size_type>(-1)) const {
        if (offset > size_) {
            throw std::out_of_range("SafeSpan::subspan offset exceeds span size");
        }
        size_type available = size_ - offset;
        size_type actual_count = (count == static_cast<size_type>(-1) || count > available) ? available : count;
        return SafeSpan<T>(data_ + offset, actual_count);
    }

    [[nodiscard]] core::Status copy_to(SafeSpan<std::remove_const_t<T>> dest) const noexcept {
        if (dest.size() < size_) {
            return core::Status(core::StatusCode::InvalidArgument, "Destination SafeSpan too small for copy");
        }
        for (size_type i = 0; i < size_; ++i) {
            dest[i] = data_[i];
        }
        return core::Status::Ok();
    }

private:
    pointer data_;
    size_type size_;
};

/// Owning safe buffer with verified capacity and guaranteed bounds checking
template <typename T>
class SafeBuffer {
public:
    using value_type = T;
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;

    SafeBuffer() = default;

    explicit SafeBuffer(size_type count) : buffer_(count) {}

    SafeBuffer(size_type count, const T& init_val) : buffer_(count, init_val) {}

    SafeBuffer(const T* data, size_type count) {
        if (data && count > 0) {
            buffer_.assign(data, data + count);
        }
    }

    [[nodiscard]] pointer data() noexcept { return buffer_.data(); }
    [[nodiscard]] const_pointer data() const noexcept { return buffer_.data(); }
    [[nodiscard]] size_type size() const noexcept { return buffer_.size(); }
    [[nodiscard]] size_type size_bytes() const noexcept { return buffer_.size() * sizeof(T); }
    [[nodiscard]] bool empty() const noexcept { return buffer_.empty(); }

    void resize(size_type new_size) {
        buffer_.resize(new_size);
    }

    void reserve(size_type capacity) {
        buffer_.reserve(capacity);
    }

    void clear() noexcept {
        buffer_.clear();
    }

    [[nodiscard]] reference at(size_type index) {
        if (index >= buffer_.size()) {
            throw std::out_of_range("SafeBuffer::at index out of range: " + std::to_string(index) + " >= " + std::to_string(buffer_.size()));
        }
        return buffer_[index];
    }

    [[nodiscard]] const_reference at(size_type index) const {
        if (index >= buffer_.size()) {
            throw std::out_of_range("SafeBuffer::at index out of range: " + std::to_string(index) + " >= " + std::to_string(buffer_.size()));
        }
        return buffer_[index];
    }

    [[nodiscard]] reference operator[](size_type index) {
        return at(index);
    }

    [[nodiscard]] const_reference operator[](size_type index) const {
        return at(index);
    }

    void append(const T& value) {
        buffer_.push_back(value);
    }

    [[nodiscard]] core::Status append_range(SafeSpan<const T> span) {
        buffer_.insert(buffer_.end(), span.begin(), span.end());
        return core::Status::Ok();
    }

    [[nodiscard]] SafeSpan<T> as_span() noexcept {
        return SafeSpan<T>(buffer_.data(), buffer_.size());
    }

    [[nodiscard]] SafeSpan<const T> as_span() const noexcept {
        return SafeSpan<const T>(buffer_.data(), buffer_.size());
    }

private:
    std::vector<T> buffer_;
};

} // namespace pingdart::safety

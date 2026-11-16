#pragma once

#include <string>
#include <memory>
#include <cstdint>

namespace pingdart::ui {

class Key {
public:
    virtual ~Key() = default;
    [[nodiscard]] virtual bool equals(const Key& other) const noexcept = 0;
};

inline bool operator==(const Key& lhs, const Key& rhs) {
    return lhs.equals(rhs);
}

inline bool keys_match(const Key* a, const Key* b) noexcept {
    if (a == b) return true;
    if (a == nullptr || b == nullptr) return false;
    return a->equals(*b);
}

template <typename T>
class ValueKey final : public Key {
public:
    explicit ValueKey(T value) : value_(std::move(value)) {}

    [[nodiscard]] const T& value() const noexcept { return value_; }

    [[nodiscard]] bool equals(const Key& other) const noexcept override {
        const auto* o = dynamic_cast<const ValueKey<T>*>(&other);
        return o && (value_ == o->value_);
    }

private:
    T value_;
};

using StringKey = ValueKey<std::string>;
using IntKey = ValueKey<int64_t>;

} // namespace pingdart::ui

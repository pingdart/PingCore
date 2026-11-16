#pragma once

#include <cstdint>
#include <functional>

namespace pingdart::web {

// Opaque element identity handle for DOM nodes.
// Avoids exposing browser raw pointers or direct JavaScript heap references to C++.
struct DomNodeHandle {
    uint64_t id{0};

    [[nodiscard]] constexpr bool is_valid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr bool operator==(const DomNodeHandle&) const noexcept = default;
};

inline constexpr DomNodeHandle kInvalidDomNodeHandle{0};

} // namespace pingdart::web

namespace std {
template <>
struct hash<pingdart::web::DomNodeHandle> {
    size_t operator()(const pingdart::web::DomNodeHandle& h) const noexcept {
        return std::hash<uint64_t>{}(h.id);
    }
};
} // namespace std

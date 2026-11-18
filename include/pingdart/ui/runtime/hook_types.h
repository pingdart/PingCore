// include/pingdart/ui/runtime/hook_types.h
// PingDartUI Hook Dependency Types (PKG-138)
#pragma once

#include <any>
#include <vector>
#include <functional>

namespace pingdart::ui {

// Opaque dependency value for useCallback / useMemo / useEffect.
// Wraps std::any so callers can pass any copyable type as a dep.
struct HookDep {
    std::any value;

    HookDep() = default;

    template <typename T>
    explicit HookDep(T&& v) : value(std::forward<T>(v)) {}

    bool operator==(const HookDep& other) const {
        // Equality only holds when both hold the same type and value.
        // For types that do not support std::any comparison we fall back
        // to pointer identity of the type_info descriptor.
        if (value.type() != other.value.type()) return false;

        // Try a few concrete types that are commonly used as deps.
        if (auto* a = std::any_cast<int>(&value)) {
            if (auto* b = std::any_cast<int>(&other.value)) return *a == *b;
        }
        if (auto* a = std::any_cast<std::size_t>(&value)) {
            if (auto* b = std::any_cast<std::size_t>(&other.value)) return *a == *b;
        }
        if (auto* a = std::any_cast<bool>(&value)) {
            if (auto* b = std::any_cast<bool>(&other.value)) return *a == *b;
        }
        if (auto* a = std::any_cast<double>(&value)) {
            if (auto* b = std::any_cast<double>(&other.value)) return *a == *b;
        }
        if (auto* a = std::any_cast<std::string>(&value)) {
            if (auto* b = std::any_cast<std::string>(&other.value)) return *a == *b;
        }

        // For pointer deps, compare the raw address.
        if (auto* a = std::any_cast<void*>(&value)) {
            if (auto* b = std::any_cast<void*>(&other.value)) return *a == *b;
        }

        // Unknown type: conservatively treat as changed so the
        // callback is refreshed rather than stale.
        return false;
    }

    bool operator!=(const HookDep& other) const { return !(*this == other); }
};

// Ordered list of hook dependencies.
using HookDeps = std::vector<HookDep>;

} // namespace pingdart::ui

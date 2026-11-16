#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <stdexcept>

namespace pingdart::web {

// Generates deterministic, hierarchical, stable `data-pd-id` attributes for SSR and client hydration.
// Incorporates the parent identity and an escaped local key:
//   pd_root
//   pd_root_k_doctor_card
//   pd_root_k_doctor_card_k_book_button
//   pd_root_0_1
// Eliminates global key collisions across different subtrees.
[[nodiscard]] inline std::string make_deterministic_id(
    std::string_view parent_id,
    size_t child_index,
    std::string_view explicit_key = "") {
    std::string base;
    if (parent_id.empty()) {
        if (!explicit_key.empty()) {
            base = "pd_root_k_";
            for (char c : explicit_key) {
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-') {
                    base.push_back(c);
                } else {
                    base.push_back('_');
                }
            }
            return base;
        }
        return "pd_root";
    }

    base = std::string(parent_id);

    if (!explicit_key.empty()) {
        base.append("_k_");
        for (char c : explicit_key) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-') {
                base.push_back(c);
            } else {
                base.push_back('_');
            }
        }
        return base;
    }

    base.push_back('_');
    base.append(std::to_string(child_index));
    return base;
}

// Sibling Key Validator: detects duplicate keys among siblings
class SiblingKeyTracker {
public:
    // Returns true if key is valid (unique among siblings or empty)
    bool track(std::string_view key) {
        if (key.empty()) return true;
        auto [it, inserted] = seen_keys_.insert(std::string(key));
        return inserted;
    }

    [[nodiscard]] bool has_duplicate() const noexcept { return has_duplicate_; }

    void record_duplicate() noexcept { has_duplicate_ = true; }

    void reset() {
        seen_keys_.clear();
        has_duplicate_ = false;
    }

private:
    std::unordered_set<std::string> seen_keys_;
    bool has_duplicate_{false};
};

} // namespace pingdart::web

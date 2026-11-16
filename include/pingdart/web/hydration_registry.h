#pragma once

#include "dom_node_handle.h"
#include "dom_event.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pingdart::web {

class HydrationRegistry {
public:
    HydrationRegistry() = default;

    // Registers a pre-rendered DOM node found during hydration scan
    void register_ssr_node(const std::string& pd_id, DomNodeHandle handle) {
        if (!pd_id.empty() && handle.is_valid()) {
            ssr_nodes_[pd_id] = handle;
        }
    }

    // Returns existing SSR handle if available, marking it as reused/hydrated
    [[nodiscard]] DomNodeHandle claim_ssr_node(const std::string& pd_id) {
        auto it = ssr_nodes_.find(pd_id);
        if (it != ssr_nodes_.end()) {
            DomNodeHandle handle = it->second;
            hydrated_nodes_.insert(handle);
            return handle;
        }
        return kInvalidDomNodeHandle;
    }

    [[nodiscard]] bool is_hydrated(DomNodeHandle handle) const noexcept {
        return hydrated_nodes_.find(handle) != hydrated_nodes_.end();
    }

    // Attach event listener tracking
    void track_listener(DomNodeHandle node, DomEventType event_type) {
        if (node.is_valid()) {
            active_listeners_[node].push_back(event_type);
        }
    }

    // Cleans up tracked event listeners when node is unmounted
    std::vector<DomEventType> release_listeners(DomNodeHandle node) {
        auto it = active_listeners_.find(node);
        if (it != active_listeners_.end()) {
            std::vector<DomEventType> released = std::move(it->second);
            active_listeners_.erase(it);
            return released;
        }
        return {};
    }

    [[nodiscard]] size_t ssr_node_count() const noexcept { return ssr_nodes_.size(); }
    [[nodiscard]] size_t hydrated_count() const noexcept { return hydrated_nodes_.size(); }
    [[nodiscard]] size_t active_listener_nodes_count() const noexcept { return active_listeners_.size(); }

    void clear() {
        ssr_nodes_.clear();
        hydrated_nodes_.clear();
        active_listeners_.clear();
    }

private:
    std::unordered_map<std::string, DomNodeHandle> ssr_nodes_;
    std::unordered_set<DomNodeHandle> hydrated_nodes_;
    std::unordered_map<DomNodeHandle, std::vector<DomEventType>> active_listeners_;
};

} // namespace pingdart::web

#pragma once

#include <pingdart/web/i_dom_adapter.h>
#include <pingdart/web/hydration_registry.h>
#include <pingdart/web/semantic_widget.h>
#include <pingdart/web/deterministic_id.h>
#include <memory>
#include <string>

namespace pingdart::web {

class DomReconciler {
public:
    DomReconciler(IDomAdapter& adapter, HydrationRegistry& registry);
    ~DomReconciler() = default;

    // Hydrates or mounts a SemanticWidget tree into the DOM under parent
    DomNodeHandle mount_or_hydrate(
        const SemanticWidget& widget,
        DomNodeHandle parent,
        std::string_view parent_pd_id = "",
        size_t child_index = 0);

    // Unmounts a node, cleaning up tracked event listeners and removing it from DOM
    void unmount(DomNodeHandle node, DomNodeHandle parent);

    [[nodiscard]] size_t nodes_reused_count() const noexcept { return nodes_reused_; }
    [[nodiscard]] size_t nodes_created_count() const noexcept { return nodes_created_; }
    [[nodiscard]] size_t listeners_attached_count() const noexcept { return listeners_attached_; }
    [[nodiscard]] size_t mismatches_recovered_count() const noexcept { return mismatches_recovered_; }

private:
    std::string get_expected_tag(const SemanticWidget& widget) const;

    IDomAdapter& adapter_;
    HydrationRegistry& registry_;
    size_t nodes_reused_{0};
    size_t nodes_created_{0};
    size_t listeners_attached_{0};
    size_t mismatches_recovered_{0};
};

} // namespace pingdart::web

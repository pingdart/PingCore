#include "dom_reconciler.h"

namespace pingdart::web {

DomReconciler::DomReconciler(IDomAdapter& adapter, HydrationRegistry& registry)
    : adapter_(adapter), registry_(registry) {}

std::string DomReconciler::get_expected_tag(const SemanticWidget& widget) const {
    if (dynamic_cast<const MainWidget*>(&widget)) return "main";
    if (dynamic_cast<const ArticleWidget*>(&widget)) return "article";
    if (dynamic_cast<const NavWidget*>(&widget)) return "nav";
    if (dynamic_cast<const ButtonWidget*>(&widget)) return "button";
    if (dynamic_cast<const LinkWidget*>(&widget)) return "a";
    if (dynamic_cast<const ParagraphWidget*>(&widget)) return "p";
    if (dynamic_cast<const SpanWidget*>(&widget)) return "span";
    if (dynamic_cast<const ImageWidget*>(&widget)) return "img";
    if (dynamic_cast<const TextFieldWidget*>(&widget)) return "input";
    if (dynamic_cast<const HeadingWidget*>(&widget)) return "h1";
    if (dynamic_cast<const FlexWidget*>(&widget)) return "div";
    return "div";
}

DomNodeHandle DomReconciler::mount_or_hydrate(
    const SemanticWidget& widget,
    DomNodeHandle parent,
    std::string_view parent_pd_id,
    size_t child_index) {
    std::string deterministic_id = make_deterministic_id(parent_pd_id, child_index, widget.key());

    // 1. Attempt to claim pre-rendered SSR node
    DomNodeHandle node = registry_.claim_ssr_node(deterministic_id);
    std::string expected_tag = get_expected_tag(widget);

    if (node.is_valid()) {
        std::string actual_tag = adapter_.get_tag_name(node);
        if (actual_tag != expected_tag) {
            // Hydration Mismatch Recovery:
            // Recover at the smallest safe subtree rather than destroying the page!
            adapter_.remove_node(node);
            node = adapter_.create_element(expected_tag);
            adapter_.set_attribute(node, "data-pd-id", deterministic_id);
            if (parent.is_valid()) {
                adapter_.append_child(parent, node);
            }
            mismatches_recovered_++;
        } else {
            nodes_reused_++;
        }
    } else {
        node = adapter_.create_element(expected_tag);
        adapter_.set_attribute(node, "data-pd-id", deterministic_id);

        if (parent.is_valid()) {
            adapter_.append_child(parent, node);
        }
        nodes_created_++;
    }

    // 2. Attach interactive listeners (e.g. Button click)
    if (dynamic_cast<const ButtonWidget*>(&widget)) {
        adapter_.add_event_listener(node, DomEventType::Click);
        registry_.track_listener(node, DomEventType::Click);
        listeners_attached_++;
    } else if (dynamic_cast<const TextFieldWidget*>(&widget)) {
        adapter_.add_event_listener(node, DomEventType::Input);
        registry_.track_listener(node, DomEventType::Input);
        listeners_attached_++;
    }

    // 3. Mount children recursively
    for (size_t i = 0; i < widget.child_count(); ++i) {
        const auto* child = widget.get_child(i);
        if (child) {
            mount_or_hydrate(*child, node, deterministic_id, i);
        }
    }

    return node;
}

void DomReconciler::unmount(DomNodeHandle node, DomNodeHandle parent) {
    if (!node.is_valid()) return;

    // Release all active listeners
    auto released_listeners = registry_.release_listeners(node);
    for (auto type : released_listeners) {
        adapter_.remove_event_listener(node, type);
    }

    // Remove from DOM
    if (parent.is_valid()) {
        adapter_.remove_child(parent, node);
    } else {
        adapter_.remove_node(node);
    }
}

} // namespace pingdart::web

#pragma once

#include <pingdart/web/i_dom_adapter.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace pingdart::web {

struct MockDomNode {
    DomNodeHandle handle{kInvalidDomNodeHandle};
    std::string tag;
    std::string text_content;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<DomNodeHandle> children;
    std::unordered_set<DomEventType> listeners;
};

class MockDomAdapter final : public IDomAdapter {
public:
    MockDomAdapter() = default;
    ~MockDomAdapter() override = default;

    DomNodeHandle create_element(std::string_view tag) override;
    DomNodeHandle create_text_node(std::string_view text) override;
    void remove_node(DomNodeHandle node) override;

    void append_child(DomNodeHandle parent, DomNodeHandle child) override;
    void insert_before(DomNodeHandle parent, DomNodeHandle new_child, DomNodeHandle ref_child) override;
    void remove_child(DomNodeHandle parent, DomNodeHandle child) override;

    void set_attribute(DomNodeHandle node, std::string_view name, std::string_view value) override;
    void remove_attribute(DomNodeHandle node, std::string_view name) override;
    void set_text_content(DomNodeHandle node, std::string_view text) override;

    [[nodiscard]] std::string get_attribute(DomNodeHandle node, std::string_view name) const override;
    [[nodiscard]] std::string get_text_content(DomNodeHandle node) const override;
    [[nodiscard]] std::string get_tag_name(DomNodeHandle node) const override;

    void add_event_listener(DomNodeHandle node, DomEventType type) override;
    void remove_event_listener(DomNodeHandle node, DomEventType type) override;
    void focus(DomNodeHandle node) override;
    [[nodiscard]] DomNodeHandle active_element() const noexcept override { return active_element_; }

    [[nodiscard]] bool has_listener(DomNodeHandle node, DomEventType type) const;
    [[nodiscard]] const std::vector<DomNodeHandle>& get_children(DomNodeHandle parent) const;
    [[nodiscard]] size_t node_count() const noexcept { return nodes_.size(); }

private:
    uint64_t next_id_{1};
    DomNodeHandle active_element_{kInvalidDomNodeHandle};
    std::unordered_map<DomNodeHandle, MockDomNode> nodes_;
    static inline const std::vector<DomNodeHandle> kEmptyChildren{};
};

} // namespace pingdart::web

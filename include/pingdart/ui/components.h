// include/pingdart/ui/components.h
// PingDartUI Production Component Library (PKG-137)
// Complete 70-Component Cross-Platform Native & Web Catalog
#pragma once

#include <pingdart/ui/component.h>
#include <pingdart/ui/router.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <initializer_list>
#include <functional>
#include <algorithm>
#include <memory>

namespace pingdart::ui {

// ============================================================================
// 1. Layout Components (7)
// ============================================================================

// 1.1 Container — Primary flexible container with column flow by default
inline Component Container(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .flexDirection(FlexDirection::Column)
        .width("100%");
}

inline Component Container(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Column)
        .width("100%");
}

// 1.2 Row — Horizontal flex layout with centered vertical alignment
inline Component Row(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center);
}

inline Component Row(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center);
}

// 1.3 Column — Vertical flex layout
inline Component Column(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .flexDirection(FlexDirection::Column);
}

inline Component Column(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Column);
}

// 1.4 Stack — Layered container for z-index or absolute overlay positioning
inline Component Stack(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .position("relative")
        .display("flex")
        .flexDirection(FlexDirection::Column);
}

inline Component Stack(std::vector<Component> children) {
    return Component("div", std::move(children))
        .position("relative")
        .display("flex")
        .flexDirection(FlexDirection::Column);
}

// 1.5 Flex — Generic flex container with full layout property control
inline Component Flex(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .display("flex");
}

inline Component Flex(std::vector<Component> children) {
    return Component("div", std::move(children))
        .display("flex");
}

// 1.6 Grid — CSS Grid layout container
inline Component Grid(const std::string& cols, float gap_px, std::initializer_list<Component> children) {
    return Component("div", children)
        .grid(cols, gap_px);
}

inline Component Grid(const std::string& cols, float gap_px, std::vector<Component> children) {
    return Component("div", std::move(children))
        .grid(cols, gap_px);
}

inline Component Grid(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .grid("repeat(auto-fit, minmax(200px, 1fr))", 16.0f);
}

inline Component Grid(std::vector<Component> children) {
    return Component("div", std::move(children))
        .grid("repeat(auto-fit, minmax(200px, 1fr))", 16.0f);
}

// 1.7 ScrollView — Scrollable container viewport with overflow clipping
inline Component ScrollView(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .overflowY("auto")
        .flexDirection(FlexDirection::Column)
        .width("100%")
        .maxHeight(600.0f);
}

inline Component ScrollView(std::vector<Component> children) {
    return Component("div", std::move(children))
        .overflowY("auto")
        .flexDirection(FlexDirection::Column)
        .width("100%")
        .maxHeight(600.0f);
}

// ============================================================================
// 2. Basic Components (6)
// ============================================================================

// 2.1 Text — Semantic text span or reactive string binding
inline Component Text(const std::string& content) {
    return Component("span", content);
}

inline Component Text(std::function<std::string()> reactive_content) {
    return Component("span", std::move(reactive_content));
}

// 2.2 Heading — Semantic heading (h1–h6) with proportional typographic hierarchy
inline Component Heading(const std::string& content, int level = 1) {
    int lvl = std::clamp(level, 1, 6);
    std::string tag = "h" + std::to_string(lvl);
    auto comp = Component(tag, content);
    if (lvl == 1) comp.fontSize(32).weight(FontWeight::Bold);
    else if (lvl == 2) comp.fontSize(24).weight(FontWeight::Bold);
    else if (lvl == 3) comp.fontSize(20).weight(FontWeight::SemiBold);
    else if (lvl == 4) comp.fontSize(18).weight(FontWeight::SemiBold);
    else comp.fontSize(16).weight(FontWeight::Medium);
    return comp;
}

// 2.3 Image — Image tag with src, alt, and display formatting
inline Component Image(const std::string& src, const std::string& alt = "") {
    return Component("img").src(src).alt(alt);
}

// 2.4 Icon — Accessible icon tag with data-icon and icon class name
inline Component Icon(const std::string& name) {
    return Component("i").attr("data-icon", name).className("icon-" + name);
}

// 2.5 Divider — Horizontal rule separator
inline Component Divider() {
    return Component("hr")
        .width("100%")
        .height(1)
        .background("#E2E8F0")
        .border(0, "transparent");
}

// 2.6 Spacer — Invisible layout spacer box
inline Component Spacer(float size = 16.0f) {
    return Component("div").width(size).height(size);
}

// ============================================================================
// 3. Actions & Buttons (4)
// ============================================================================

// 3.1 Button — Interactive trigger button
inline Component Button(const std::string& label) {
    return Component("button", label)
        .padding(10, 18)
        .borderRadius(8)
        .fontSize(15)
        .weight(FontWeight::Medium)
        .background("#F1F5F9")
        .color("#1E293B")
        .cursor("pointer");
}

// 3.2 IconButton — Button containing an icon with accessible label
inline Component IconButton(const std::string& icon_name, const std::string& label = "") {
    return Component("button", { Icon(icon_name) })
        .attr("aria-label", label.empty() ? icon_name : label)
        .padding(8, 8)
        .borderRadius(8)
        .background("#F8FAFC")
        .border(1, "#E2E8F0")
        .cursor("pointer");
}

// 3.3 FloatingButton — Floating action button with elevated circular shape
inline Component FloatingButton(const std::string& icon_name, const std::string& label = "") {
    return Component("button", { Icon(icon_name) })
        .attr("aria-label", label.empty() ? icon_name : label)
        .width(56).height(56)
        .borderRadius(9999)
        .background("#2563EB")
        .color("#FFFFFF")
        .shadow(0, 4, 12, "#00000026")
        .cursor("pointer");
}

// 3.4 ButtonGroup — Grouped buttons with merged borders
inline Component ButtonGroup(std::initializer_list<Component> buttons) {
    return Component("div", buttons)
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center)
        .className("pingdart-button-group");
}

// ============================================================================
// 4. Forms & Input Controls (7)
// ============================================================================

// 4.1 Input — Text entry field
inline Component Input(const std::string& placeholder = "", const std::string& val = "") {
    auto comp = Component("input")
        .placeholder(placeholder)
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
    comp.attr("type", "text");
    if (!val.empty()) comp.value(val);
    return comp;
}

inline Component Input(const std::string& name, const std::string& type, const std::string& placeholder, const std::string& val = "") {
    auto comp = Input(placeholder, val);
    comp.attr("name", name);
    comp.attr("type", type);
    return comp;
}

// 4.2 TextArea — Multi-line text entry field
inline Component TextArea(const std::string& placeholder = "", int rows = 4) {
    return Component("textarea")
        .placeholder(placeholder)
        .attr("rows", std::to_string(rows))
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
}

// 4.3 Checkbox — Boolean toggle checkbox with label
inline Component Checkbox(const std::string& label, bool is_checked = false) {
    auto input_comp = Component("input")
        .attr("type", "checkbox")
        .checked(is_checked);
    return Component("label", { input_comp, Text(label) })
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center)
        .gap(8)
        .cursor("pointer");
}

// 4.4 Radio — Radio option button with group name and value
inline Component Radio(const std::string& name, const std::string& val, const std::string& label, bool is_checked = false) {
    auto input_comp = Component("input")
        .attr("type", "radio")
        .attr("name", name)
        .value(val)
        .checked(is_checked);
    return Component("label", { input_comp, Text(label) })
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center)
        .gap(8)
        .cursor("pointer");
}

// 4.5 Switch — Accessible toggle switch component
inline Component Switch(bool is_checked = false) {
    auto thumb = Component("div")
        .width(18)
        .height(18)
        .borderRadius(9999)
        .background("#FFFFFF")
        .shadow(0, 1, 2, "#00000026");
    if (is_checked) {
        thumb.margin(0, 0, 0, 18);
    }

    return Component("button", { thumb })
        .role("switch")
        .attr("aria-checked", is_checked ? "true" : "false")
        .width(42)
        .height(24)
        .borderRadius(9999)
        .background(is_checked ? "#2563EB" : "#CBD5E1")
        .border(0, "transparent")
        .padding(3)
        .cursor("pointer");
}

// 4.6 Option & Select — Dropdown menu
inline Component Option(const std::string& label, const std::string& val = "") {
    auto opt = Component("option", label);
    opt.value(val.empty() ? label : val);
    return opt;
}

inline Component Select(std::initializer_list<Component> options) {
    return Component("select", options)
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
}

inline Component Select(std::vector<Component> options) {
    return Component("select", std::move(options))
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
}

// 4.7 Slider — Range slider control
inline Component Slider(float min_val = 0.0f, float max_val = 100.0f, float cur_val = 50.0f) {
    auto comp = Component("input")
        .attr("type", "range")
        .attr("min", std::to_string(static_cast<int>(min_val)))
        .attr("max", std::to_string(static_cast<int>(max_val)))
        .attr("value", std::to_string(static_cast<int>(cur_val)))
        .width("100%")
        .cursor("pointer");
    return comp;
}

// ============================================================================
// 5. Date & Time Components (3)
// ============================================================================

// 5.1 DatePicker — Date selection control
inline Component DatePicker(const std::string& selected_date = "") {
    auto comp = Component("input")
        .attr("type", "date")
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
    if (!selected_date.empty()) comp.value(selected_date);
    return comp;
}

// 5.2 TimePicker — Time selection control
inline Component TimePicker(const std::string& selected_time = "") {
    auto comp = Component("input")
        .attr("type", "time")
        .padding(10, 14)
        .borderRadius(8)
        .border(1, "#CBD5E1")
        .background("#FFFFFF")
        .fontSize(15);
    if (!selected_time.empty()) comp.value(selected_time);
    return comp;
}

// 5.3 Calendar — Monthly date view grid
inline Component Calendar(int year = 2026, int month = 11) {
    std::vector<Component> days;
    std::vector<std::string> headers = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    for (const auto& h : headers) {
        days.push_back(Text(h).weight(FontWeight::SemiBold).color("#64748B"));
    }
    for (int d = 1; d <= 30; ++d) {
        days.push_back(Text(std::to_string(d)).className("p-2 text-center rounded hover:bg-slate-100"));
    }
    return Component("div", {
        Heading(std::to_string(year) + " / " + std::to_string(month), 4),
        Grid("repeat(7, 1fr)", 4.0f, days)
    }).padding(16).borderRadius(12).border(1, "#E2E8F0").background("#FFFFFF");
}

// ============================================================================
// 6. Navigation Components (6)
// ============================================================================

// 6.1 Navbar — Top application bar
inline Component Navbar(std::initializer_list<Component> children) {
    return Component("nav", children)
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center)
        .padding(12, 24)
        .background("#FFFFFF")
        .border(1, "#E2E8F0")
        .width("100%");
}

// 6.2 Sidebar — Vertical navigation panel
inline Component Sidebar(std::initializer_list<Component> children) {
    return Component("aside", children)
        .flexDirection(FlexDirection::Column)
        .width(260)
        .padding(20, 16)
        .background("#FFFFFF")
        .border(1, "#E2E8F0")
        .gap(8);
}

// 6.3 Tabs — Accessible tabbed container
inline Component Tab(const std::string& title, const Component& content) {
    return Component("div", { Text(title), content }).attr("data-tab-title", title);
}

inline Component Tabs(std::initializer_list<Component> tab_items) {
    return Component("div", tab_items)
        .role("tablist")
        .flexDirection(FlexDirection::Column)
        .width("100%");
}

// 6.4 Drawer — Slide-out side drawer overlay
inline Component Drawer(std::initializer_list<Component> children, bool is_open = false) {
    if (!is_open) return Component("aside").display("none");
    return Component("aside", children)
        .position("relative")
        .width(320)
        .padding(24)
        .background("#FFFFFF")
        .shadow(4, 0, 16, "#0000001A");
}

// 6.5 Breadcrumb — Path navigation trail
inline Component Breadcrumb(const std::vector<std::string>& path_segments) {
    std::vector<Component> items;
    for (size_t i = 0; i < path_segments.size(); ++i) {
        if (i > 0) items.push_back(Text("/").color("#94A3B8"));
        items.push_back(Text(path_segments[i]).color(i + 1 == path_segments.size() ? "#0F172A" : "#2563EB"));
    }
    return Component("nav", items)
        .role("navigation")
        .attr("aria-label", "Breadcrumb")
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center)
        .gap(8);
}

// 6.6 BottomNavigation — Mobile-friendly bottom action bar
inline Component BottomNavigation(std::initializer_list<Component> items) {
    return Component("nav", items)
        .flexDirection(FlexDirection::Row)
        .justifyContent(JustifyContent::SpaceAround)
        .alignItems(AlignItems::Center)
        .padding(10, 16)
        .background("#FFFFFF")
        .border(1, "#E2E8F0")
        .width("100%");
}

// ============================================================================
// 7. Display & Structured Data Components (7)
// ============================================================================

// 7.1 Card — Elevated container surface
inline Component Card(std::initializer_list<Component> children = {}) {
    return Component("div", children)
        .flexDirection(FlexDirection::Column)
        .background("#FFFFFF")
        .borderRadius(16)
        .border(1, "#E2E8F0")
        .padding(20)
        .gap(12)
        .shadow(0, 4, 6, "#0000000D");
}

inline Component Card(std::vector<Component> children) {
    return Component("div", std::move(children))
        .flexDirection(FlexDirection::Column)
        .background("#FFFFFF")
        .borderRadius(16)
        .border(1, "#E2E8F0")
        .padding(20)
        .gap(12)
        .shadow(0, 4, 6, "#0000000D");
}

// 7.2 Avatar — User or entity circular avatar
inline Component Avatar(const std::string& src_or_initials, bool is_image = false, float size = 40.0f) {
    if (is_image) {
        return Component("img")
            .src(src_or_initials)
            .width(size)
            .height(size)
            .borderRadius(9999);
    } else {
        return Component("div", { Text(src_or_initials).weight(FontWeight::SemiBold).fontSize(size * 0.4f).color("#1E293B") })
            .width(size)
            .height(size)
            .borderRadius(9999)
            .background("#E2E8F0")
            .display("flex")
            .alignItems(AlignItems::Center)
            .justifyContent(JustifyContent::Center);
    }
}

// 7.3 Badge — Compact status pill
inline Component Badge(const std::string& label) {
    return Component("span", label)
        .padding(4, 10)
        .borderRadius(9999)
        .fontSize(12)
        .weight(FontWeight::Medium)
        .background("#EFF6FF")
        .color("#2563EB");
}

// 7.4 Table & TableRow / TableCell
inline Component TableCell(const std::string& text_val, bool is_header = false) {
    return Component(is_header ? "th" : "td", text_val)
        .padding(10, 14)
        .border(1, "#E2E8F0")
        .textAlign(TextAlign::Left);
}

inline Component TableRow(std::initializer_list<Component> cells) {
    return Component("tr", cells);
}

inline Component TableRow(std::vector<Component> cells) {
    return Component("tr", std::move(cells));
}

inline Component Table(std::initializer_list<Component> rows) {
    return Component("table", rows)
        .width("100%")
        .border(1, "#E2E8F0")
        .borderRadius(8);
}

inline Component Table(std::vector<Component> rows) {
    return Component("table", std::move(rows))
        .width("100%")
        .border(1, "#E2E8F0")
        .borderRadius(8);
}

// 7.5 ColumnDef & DataGrid
struct ColumnDef {
    std::string key{""};
    std::string label{""};
    bool sortable{true};

    ColumnDef() = default;
    ColumnDef(std::string k, std::string l, bool s = true)
        : key(std::move(k)), label(std::move(l)), sortable(s) {}
};

class DataGridBuilder : public StyleBuilder<DataGridBuilder> {
private:
    std::vector<ColumnDef> columns_{};
    std::vector<std::unordered_map<std::string, std::string>> data_{};
    bool searchable_{false};
    bool sortable_{false};
    int pagination_{0};
    int current_page_{1};
    std::string sort_column_{""};
    bool sort_ascending_{true};
    std::string filter_query_{""};
    int selected_index_{-1};

public:
    DataGridBuilder& columns(std::vector<ColumnDef> cols) {
        columns_ = std::move(cols);
        return *this;
    }

    DataGridBuilder& data(std::vector<std::unordered_map<std::string, std::string>> d) {
        data_ = std::move(d);
        return *this;
    }

    DataGridBuilder& searchable(bool s = true) {
        searchable_ = s;
        return *this;
    }

    DataGridBuilder& sortable(bool s = true) {
        sortable_ = s;
        return *this;
    }

    DataGridBuilder& pagination(int p) {
        pagination_ = p;
        return *this;
    }

    DataGridBuilder& page(int p) {
        current_page_ = std::max(1, p);
        return *this;
    }

    DataGridBuilder& sortBy(std::string col, bool asc = true) {
        sort_column_ = std::move(col);
        sort_ascending_ = asc;
        return *this;
    }

    DataGridBuilder& filter(std::string query) {
        filter_query_ = std::move(query);
        return *this;
    }

    DataGridBuilder& select(size_t index) {
        selected_index_ = static_cast<int>(index);
        return *this;
    }

    size_t row_count() const noexcept {
        return data_.size();
    }

    Component build() const {
        auto rows_to_display = data_;

        // 1. In-Memory Filtering
        if (!filter_query_.empty()) {
            std::vector<std::unordered_map<std::string, std::string>> filtered;
            for (const auto& row : rows_to_display) {
                bool match = false;
                for (const auto& [k, v] : row) {
                    if (v.find(filter_query_) != std::string::npos) {
                        match = true;
                        break;
                    }
                }
                if (match) filtered.push_back(row);
            }
            rows_to_display = std::move(filtered);
        }

        // 2. In-Memory Sorting
        if (!sort_column_.empty()) {
            std::sort(rows_to_display.begin(), rows_to_display.end(),
                [this](const std::unordered_map<std::string, std::string>& a,
                       const std::unordered_map<std::string, std::string>& b) {
                    auto it_a = a.find(sort_column_);
                    auto it_b = b.find(sort_column_);
                    std::string val_a = (it_a != a.end()) ? it_a->second : "";
                    std::string val_b = (it_b != b.end()) ? it_b->second : "";
                    return sort_ascending_ ? (val_a < val_b) : (val_a > val_b);
                });
        }

        size_t total_rows = rows_to_display.size();

        // 3. In-Memory Pagination
        size_t start_idx = 0;
        size_t end_idx = total_rows;
        if (pagination_ > 0) {
            size_t pag_sz = static_cast<size_t>(std::max(1, pagination_));
            start_idx = static_cast<size_t>(std::max(0, current_page_ - 1)) * pag_sz;
            if (start_idx > total_rows) start_idx = total_rows;
            end_idx = std::min(start_idx + pag_sz, total_rows);
        }

        std::vector<Component> table_rows;

        // Header Row
        std::vector<Component> header_cells;
        for (const auto& col : columns_) {
            std::string indicator = "";
            if (sortable_ && col.sortable) {
                if (sort_column_ == col.key) {
                    indicator = sort_ascending_ ? " ▲" : " ▼";
                } else {
                    indicator = " ⇅";
                }
            }
            header_cells.push_back(TableCell(col.label + indicator, true));
        }
        table_rows.push_back(TableRow(header_cells));

        // Data Rows
        for (size_t r = start_idx; r < end_idx; ++r) {
            const auto& row_map = rows_to_display[r];
            std::vector<Component> row_cells;
            for (const auto& col : columns_) {
                auto it = row_map.find(col.key);
                std::string cell_val = (it != row_map.end()) ? it->second : "";
                row_cells.push_back(TableCell(cell_val, false));
            }
            auto row_comp = TableRow(row_cells);
            if (selected_index_ >= 0 && static_cast<size_t>(selected_index_) == r) {
                row_comp.background("#EFF6FF");
            }
            table_rows.push_back(row_comp);
        }

        auto table_comp = Component("table", table_rows).width("100%");

        std::vector<Component> container_children;
        if (searchable_) {
            container_children.push_back(Input("Search table records...", filter_query_).className("mb-3"));
        }
        container_children.push_back(table_comp);

        if (pagination_ > 0) {
            container_children.push_back(
                Row({
                    Text("Showing " + std::to_string(end_idx - start_idx) + " of " + std::to_string(total_rows) + " rows (Page " + std::to_string(current_page_) + ")"),
                    Spacer(),
                    Button("Previous").padding(4, 10),
                    Button("Next").padding(4, 10)
                }).padding(8, 0)
            );
        }

        auto root = Component("div", container_children)
            .className("pingdart-datagrid")
            .border(1, "#E2E8F0")
            .borderRadius(12)
            .padding(16)
            .background("#FFFFFF");
        root.computed_style() = style_;
        return root;
    }

    operator Component() const {
        return build();
    }
};

inline DataGridBuilder DataGrid() {
    return DataGridBuilder();
}

// 7.6 List & ListItem
inline Component ListItem(const std::string& item_text) {
    return Component("li", item_text).padding(6, 12);
}

inline Component List(std::initializer_list<Component> items) {
    return Component("ul", items).padding(0).margin(0).className("pingdart-list");
}

// 7.7 Tree & TreeNode
inline Component TreeNode(const std::string& label, std::initializer_list<Component> subnodes = {}) {
    std::vector<Component> node_children = { Text("▸ " + label) };
    if (subnodes.size() > 0) {
        node_children.push_back(Component("div", subnodes).padding(0, 0, 0, 16));
    }
    return Component("div", node_children).padding(4, 8);
}

inline Component Tree(std::initializer_list<Component> nodes) {
    return Component("div", nodes).flexDirection(FlexDirection::Column);
}

// ============================================================================
// 8. Feedback & Progress Components (6)
// ============================================================================

// 8.1 Alert — Notice and warning box
inline Component Alert(const std::string& message) {
    return Component("div", { Text(message) })
        .padding(14, 18)
        .borderRadius(8)
        .background("#FEF2F2")
        .border(1, "#FECACA")
        .color("#991B1B");
}

// 8.2 Toast — Floating message notification
inline Component Toast(const std::string& message, const std::string& type = "info") {
    std::string bg = (type == "success") ? "#ECFDF5" : ((type == "error") ? "#FEF2F2" : "#EFF6FF");
    std::string col = (type == "success") ? "#065F46" : ((type == "error") ? "#991B1B" : "#1E40AF");
    return Component("div", { Text(message) })
        .padding(12, 16)
        .borderRadius(8)
        .background(bg)
        .color(col)
        .shadow(0, 4, 12, "#0000001A");
}

// 8.3 Snackbar — Bottom snackbar with optional action button
inline Component Snackbar(const std::string& message, const std::string& action_label = "") {
    std::vector<Component> items = { Text(message).color("#FFFFFF") };
    if (!action_label.empty()) {
        items.push_back(Spacer());
        items.push_back(Button(action_label).color("#60A5FA").background("transparent").border(0, "transparent"));
    }
    return Component("div", items)
        .flexDirection(FlexDirection::Row)
        .alignItems(AlignItems::Center)
        .padding(12, 20)
        .borderRadius(8)
        .background("#1E293B")
        .shadow(0, 4, 16, "#00000033");
}

// 8.4 ProgressBar — Visual progress meter
inline Component ProgressBar(float percent) {
    float clamped = std::clamp(percent, 0.0f, 100.0f);
    auto fill = Component("div")
        .width(std::to_string(static_cast<int>(clamped)) + "%")
        .height(8)
        .background("#2563EB")
        .borderRadius(9999);
    return Component("div", { fill })
        .width("100%")
        .height(8)
        .background("#E2E8F0")
        .borderRadius(9999)
        .overflow("hidden");
}

// 8.5 Spinner — Circular loading indicator
inline Component Spinner(float size = 24.0f) {
    return Component("div")
        .width(size).height(size)
        .borderRadius(9999)
        .border(3, "#E2E8F0")
        .attr("data-role", "spinner")
        .className("pingdart-spinner");
}

// 8.6 Skeleton — Shimmering placeholder box
inline Component Skeleton(const std::string& w = "100%", float h = 20.0f) {
    return Component("div")
        .width(w).height(h)
        .background("#E2E8F0")
        .borderRadius(6)
        .className("pingdart-skeleton");
}

// ============================================================================
// 9. Overlays & Dialogs (5)
// ============================================================================

// 9.1 Dialog — Modal dialog with backdrop and central card
inline Component Dialog(const std::string& title, const Component& body, bool is_open = true) {
    if (!is_open) return Component("div").display("none");
    auto modal_content = Card({
        Row({ Heading(title, 3), Spacer(), IconButton("close", "Close dialog") }),
        Divider(),
        body
    }).width("480px").shadow(0, 20, 25, -5, "#0000001A");

    return Component("div", { modal_content })
        .role("dialog")
        .attr("aria-modal", "true")
        .position("relative")
        .padding(32)
        .background("#00000033")
        .display("flex")
        .alignItems(AlignItems::Center)
        .justifyContent(JustifyContent::Center);
}

// 9.2 Modal — Generic modal container
inline Component Modal(const std::string& title, const Component& content, bool is_open = true) {
    return Dialog(title, content, is_open);
}

// 9.3 Popover — Anchored floating popover
inline Component Popover(const Component& trigger, const Component& content) {
    return Component("div", {
        trigger,
        Component("div", { content })
            .position("relative")
            .padding(12)
            .background("#FFFFFF")
            .borderRadius(8)
            .shadow(0, 4, 12, "#0000001A")
    });
}

// 9.4 Tooltip — Hover tip wrapper
inline Component Tooltip(const std::string& tip_text, const Component& child) {
    auto comp = child;
    comp.attr("title", tip_text);
    return comp;
}

// 9.5 BottomSheet — Mobile slide-up sheet
inline Component BottomSheet(std::initializer_list<Component> children, bool is_open = true) {
    if (!is_open) return Component("div").display("none");
    return Component("div", children)
        .position("relative")
        .width("100%")
        .padding(24)
        .background("#FFFFFF")
        .border(1, "#E2E8F0")
        .borderRadius(16)
        .shadow(0, -4, 16, "#0000001A");
}

// ============================================================================
// 10. Application Shell & Views (5)
// ============================================================================

// 10.1 AppShell — Master responsive shell
inline Component AppShell(std::initializer_list<Component> children) {
    return Component("div", children)
        .display("flex")
        .flexDirection(FlexDirection::Row)
        .width("100%")
        .height("100%");
}

// 10.2 Page — Primary content view container
inline Component Page(std::initializer_list<Component> children) {
    return Component("main", children)
        .display("flex")
        .flexDirection(FlexDirection::Column)
        .width("100%")
        .padding(24)
        .gap(20);
}

// 10.3 Outlet — Routing anchor
inline Component Outlet() {
    return Component("div").attr("data-role", "router-outlet");
}

// 10.4 SafeArea — Mobile safe area padding container
inline Component SafeArea(std::initializer_list<Component> children) {
    return Component("div", children)
        .padding(24, 16, 34, 16)
        .width("100%");
}

// 10.5 StatCard — Standard KPI dashboard metric card
inline Component StatCard(const std::string& title, int value, const std::string& subtitle = "") {
    return Card({
        Text(title).color("#64748B").weight(FontWeight::Medium),
        Heading(std::to_string(value), 2).color("#0F172A"),
        Text(subtitle).color("#16A34A").fontSize(13)
    }).padding(16).borderRadius(12).border(1, "#E2E8F0");
}

// ============================================================================
// 11. Media & Advanced Components (7)
// ============================================================================

// 11.1 VideoPlayer — Video media player
inline Component VideoPlayer(const std::string& src, const std::string& poster = "") {
    auto comp = Component("video").attr("controls", "controls").src(src);
    if (!poster.empty()) comp.attr("poster", poster);
    return comp.width("100%").borderRadius(8);
}

// 11.2 AudioPlayer — Audio media player
inline Component AudioPlayer(const std::string& src) {
    return Component("audio").attr("controls", "controls").src(src).width("100%");
}

// 11.3 ImageGallery — Grid of images
inline Component ImageGallery(const std::vector<std::string>& urls) {
    std::vector<Component> imgs;
    for (const auto& u : urls) {
        imgs.push_back(Image(u).borderRadius(8).width("100%"));
    }
    return Grid("repeat(auto-fit, minmax(140px, 1fr))", 8.0f, imgs);
}

// 11.4 Charts — Bar & Line chart visualizer
inline Component Charts(const std::string& type, const std::vector<std::string>& labels, const std::vector<float>& values) {
    std::vector<Component> bars;
    float max_v = 1.0f;
    for (float v : values) if (v > max_v) max_v = v;

    for (size_t i = 0; i < values.size(); ++i) {
        float pct = (values[i] / max_v) * 100.0f;
        auto bar = Component("div")
            .width(28)
            .height(std::to_string(static_cast<int>(pct)) + "%")
            .background("#2563EB")
            .borderRadius(4);
        std::string lbl = (i < labels.size()) ? labels[i] : "";
        bars.push_back(Column({ bar, Text(lbl).fontSize(12).color("#64748B") }).alignItems(AlignItems::Center).gap(4));
    }

    return Component("div", {
        Heading(type + " Chart", 4),
        Row(bars).alignItems(AlignItems::End).height(180).gap(12).padding(12).background("#F8FAFC").borderRadius(8)
    });
}

// 11.5 RichTextEditor — Text editing surface
inline Component RichTextEditor(const std::string& placeholder = "Type here...") {
    return Component("div", {
        Row({
            Button("B").padding(4, 8),
            Button("I").padding(4, 8),
            Button("U").padding(4, 8)
        }).gap(4).padding(6).background("#F1F5F9").borderRadius(6),
        TextArea(placeholder, 6).border(0, "transparent")
    }).border(1, "#CBD5E1").borderRadius(8).background("#FFFFFF");
}

// 11.6 VirtualList — Virtualized scrolling list
inline Component VirtualList(int item_count, float item_height) {
    return Component("div")
        .attr("data-virtual-count", std::to_string(item_count))
        .attr("data-item-height", std::to_string(static_cast<int>(item_height)))
        .overflowY("auto")
        .height(300)
        .border(1, "#E2E8F0");
}

// 11.7 FilePicker — File upload selector
inline Component FilePicker(const std::string& accept = "*/*") {
    return Component("input")
        .attr("type", "file")
        .attr("accept", accept)
        .padding(8, 12)
        .border(1, "#CBD5E1")
        .borderRadius(8);
}

// ============================================================================
// 12. Compound Workflows (7)
// ============================================================================

// 12.1 Form — Accessible interactive form container
inline Component Form(std::initializer_list<Component> fields) {
    return Component("form", fields)
        .flexDirection(FlexDirection::Column)
        .gap(16)
        .width("100%");
}

inline Component Form(std::vector<Component> fields) {
    return Component("form", std::move(fields))
        .flexDirection(FlexDirection::Column)
        .gap(16)
        .width("100%");
}

// 12.2 FormField — Labeled input wrapper with optional error message
inline Component FormField(const std::string& label, const Component& input_comp, const std::string& error_msg = "") {
    std::vector<Component> items = {
        Text(label).weight(FontWeight::Medium).color("#1E293B"),
        input_comp
    };
    if (!error_msg.empty()) {
        items.push_back(Text(error_msg).color("#DC2626").fontSize(12));
    }
    return Component("div", items).flexDirection(FlexDirection::Column).gap(6).width("100%");
}

// 12.3 SearchBox — Search input with integrated clear and icon
inline Component SearchBox(const std::string& placeholder = "Search...") {
    return Row({
        Icon("search"),
        Input(placeholder).border(0, "transparent").width("100%")
    }).border(1, "#CBD5E1").borderRadius(8).padding(4, 10).background("#FFFFFF");
}

// 12.4 Pagination — Multi-page navigation control
inline Component Pagination(int total_pages = 5, int current_page = 1) {
    std::vector<Component> pages = { Button("Prev").padding(6, 12) };
    for (int p = 1; p <= total_pages; ++p) {
        auto btn = Button(std::to_string(p)).padding(6, 12);
        if (p == current_page) btn.variant("primary");
        pages.push_back(btn);
    }
    pages.push_back(Button("Next").padding(6, 12));
    return Row(pages).gap(6);
}

// 12.5 Accordion — Collapsible disclosure section
inline Component Accordion(std::initializer_list<Component> sections) {
    return Component("div", sections)
        .flexDirection(FlexDirection::Column)
        .border(1, "#E2E8F0")
        .borderRadius(8);
}

// 12.6 Carousel — Slide carousel
inline Component Carousel(std::initializer_list<Component> slides) {
    return Component("div", {
        Row(slides).overflow("hidden").width("100%"),
        Row({ Button("◀"), Spacer(), Button("▶") }).padding(8, 0)
    }).width("100%");
}

// 12.7 CommandPalette — Command search modal palette
inline Component CommandPalette(const std::string& placeholder = "Type a command or search...") {
    return Dialog("Command Palette", {
        Input(placeholder).className("w-full mb-3"),
        List({
            ListItem("Open Settings (Cmd+,)"),
            ListItem("Toggle Dark Mode (Cmd+D)"),
            ListItem("Export Patient Report (Cmd+E)")
        })
    }, true);
}

// Navigation Link helper
inline Component Link(const std::string& label, const std::string& href) {
    return Component("a", label)
        .href(href)
        .color("#2563EB")
        .onClick([href]() {
            Router::push(href);
        });
}

} // namespace pingdart::ui

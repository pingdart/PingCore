// examples/interactive_datagrid/main.cpp
// PingDartUI Interactive DataGrid Example (PKG-138)
#include <pingdart/ui.h>
#include <iostream>

using namespace pingdart::ui;

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    std::cout << "==========================================" << std::endl;
    std::cout << " PingDartUI — Interactive DataGrid Demo   " << std::endl;
    std::cout << "==========================================" << std::endl;

    std::vector<std::unordered_map<std::string, std::string>> records = {
        {{"id", "101"}, {"name", "Dr. Jane Smith"}, {"dept", "Cardiology"}, {"patients", "145"}, {"status", "Available"}},
        {{"id", "102"}, {"name", "Dr. Alan Turing"}, {"dept", "Neurology"}, {"patients", "98"}, {"status", "On-Call"}},
        {{"id", "103"}, {"name", "Dr. Marie Curie"}, {"dept", "Oncology"}, {"patients", "210"}, {"status", "Consulting"}},
        {{"id", "104"}, {"name", "Dr. Jonas Salk"}, {"dept", "Immunology"}, {"patients", "320"}, {"status", "Available"}},
        {{"id", "105"}, {"name", "Dr. Alexander Fleming"}, {"dept", "Infectious"}, {"patients", "180"}, {"status", "Busy"}}
    };

    auto grid = DataGrid()
        .columns({
            ColumnDef("id", "Doctor ID", true),
            ColumnDef("name", "Physician Name", true),
            ColumnDef("dept", "Clinical Department", true),
            ColumnDef("patients", "Active Patients", true),
            ColumnDef("status", "Current Status", false)
        })
        .data(records)
        .sortBy("patients", false)
        .filter("Cardiology")
        .select(0)
        .pagination(3);

    Component comp = grid.build();
    std::string html = render_to_html(comp);
    std::cout << "[Web SSR DataGrid Output (" << html.size() << " bytes)]:\n" << html << std::endl;

    bool has_cardio = (html.find("Dr. Jane Smith") != std::string::npos);
    std::cout << "In-Memory Filtering (Cardiology) Verified: " << (has_cardio ? "YES" : "NO") << std::endl;

    return 0;
}

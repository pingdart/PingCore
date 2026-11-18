// examples/responsive_dashboard/main.cpp
// PingDartUI Responsive Clinical Dashboard Example (PKG-138)
#include <pingdart/ui.h>
#include <iostream>

using namespace pingdart::ui;

Component ResponsiveDashboard() {
    return Container({
        Row({
            Heading("Clinical Analytics Dashboard", 1),
            Spacer(),
            Button("Export PDF").variant("secondary"),
            Button("Live Refresh").variant("primary")
        }),
        Divider(),
        Row({
            StatCard("Active Inpatients", 148, "+8% vs yesterday"),
            Spacer(12),
            StatCard("ICU Capacity", 82, "92% occupied"),
            Spacer(12),
            StatCard("Surgeries Scheduled", 24, "All operating rooms green"),
            Spacer(12),
            StatCard("Staff On Duty", 64, "Full shift coverage")
        }),
        Grid("repeat(2, 1fr)", 16.0f, {
            Card({
                Heading("Consultation Trends (H2 2026)", 3),
                Charts("Consultations", {"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}, {140.0f, 180.0f, 210.0f, 260.0f, 290.0f, 320.0f})
            }),
            Card({
                Heading("Department Patient Distribution", 3),
                List({
                    ListItem("Cardiology — 42 patients (28%)"),
                    ListItem("Neurology — 36 patients (24%)"),
                    ListItem("Pediatrics — 28 patients (19%)"),
                    ListItem("Orthopedics — 24 patients (16%)"),
                    ListItem("Emergency / Triage — 18 patients (13%)")
                })
            })
        })
    }).className("dashboard-container").padding(24).gap(20);
}

int main(int argc, char** argv) {
    std::cout << "==========================================" << std::endl;
    std::cout << " PingDartUI — Responsive Dashboard Demo   " << std::endl;
    std::cout << "==========================================" << std::endl;

    auto app = App(argc, argv).title("Clinical Dashboard").render(ResponsiveDashboard);
    app.run();

    auto dash = ResponsiveDashboard();
    std::cout << "[Web SSR Output generated: " << render_to_html(dash).size() << " bytes]" << std::endl;
    std::cout << "[Qt Quick QML generated: " << render_to_qml(dash).size() << " bytes]" << std::endl;

    return 0;
}

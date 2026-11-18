// examples/doctor_profile/main.cpp
// PingDartUI Doctor Clinical Profile Example (PKG-138)
#include <pingdart/ui.h>
#include <iostream>

using namespace pingdart::ui;

Component DoctorProfile() {
    return Card({
        Row({
            Avatar("JS", false, 56.0f),
            Column({
                Heading("Dr. Jane Smith, MD", 2),
                Text("Interventional Cardiologist · Clinical Associate Professor").color("#2563EB").weight(FontWeight::Medium),
                Text("Stanford Hospital · Department of Cardiovascular Medicine").color("#64748B")
            }).gap(4),
            Spacer(),
            Badge("Active On-Call")
        }).gap(16),
        Divider(),
        Heading("Specialties & Clinical Procedures", 4),
        List({
            ListItem("Coronary Angioplasty & Stenting"),
            ListItem("Transcatheter Aortic Valve Replacement (TAVR)"),
            ListItem("Complex Cardiovascular Disease Management")
        }),
        Divider(),
        Row({
            StatCard("Years Experience", 16, "Board Certified"),
            Spacer(12),
            StatCard("Total Procedures", 1840, "99.4% Success Rate"),
            Spacer(12),
            StatCard("Patient Rating", 5, "4.98 / 5.0 (420 reviews)")
        }),
        Divider(),
        Row({
            Button("Schedule Appointment").variant("primary").onClick([] {
                Router::push("/schedule");
            }),
            Button("Send Referral").variant("secondary").onClick([] {
                Router::push("/refer");
            })
        }).gap(12)
    }).padding(24).borderRadius(12).border(1, "#E2E8F0").background("#FFFFFF");
}

int main(int argc, char** argv) {
    std::cout << "==========================================" << std::endl;
    std::cout << " PingDartUI — Doctor Clinical Profile Demo" << std::endl;
    std::cout << "==========================================" << std::endl;

    auto app = App(argc, argv).title("Doctor Profile").render(DoctorProfile);
    app.run();

    auto profile = DoctorProfile();
    std::cout << "[Web SSR Output generated: " << render_to_html(profile).size() << " bytes]" << std::endl;
    std::cout << "[Qt Quick QML generated: " << render_to_qml(profile).size() << " bytes]" << std::endl;

    return 0;
}

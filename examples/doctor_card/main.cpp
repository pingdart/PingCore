// examples/doctor_card/main.cpp
// PingDartUI DoctorCard & Reactive Counter Example (PKG-135)
#include <pingdart/ui.h>
#include <iostream>

using namespace pingdart::ui;

Component DoctorCard() {
    return Card({
        Heading("Dr. Jane Smith, MD").size(24).weight(FontWeight::Bold),
        Text("Cardiologist").color("#2563EB").weight(FontWeight::Medium),
        Text("15 years clinical experience").color("#64748B"),
        Text("Heart Care Clinic · San Francisco, CA").color("#64748B"),
        Button("Book Consultation")
            .variant("primary")
            .onClick([] { Router::push("/booking"); }),
        Link("View Full Profile", "/doctor/profile")
    })
    .className("doctor-card")
    .background("#FFFFFF")
    .border(1, "#E2E8F0")
    .borderRadius(16)
    .padding(24)
    .gap(12)
    .shadow(0, 4, 12, "#00000010");
}

Component Counter() {
    auto count = useState<int>(0);
    return Column({
        Heading("Counter", 2),
        Text([count] { return "Current Count: " + std::to_string(count.get()); }),
        Row({
            Button("Increment")
                .variant("primary")
                .onClick([count]() mutable { count.set(count.get() + 1); }),
            Button("Reset")
                .variant("secondary")
                .onClick([count]() mutable { count.set(0); })
        }).gap(8)
    }).gap(12);
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " PingDartUI — DoctorCard Component Demo " << std::endl;
    std::cout << "========================================" << std::endl;

    auto doctor = DoctorCard();
    std::cout << "\n[Web SSR HTML Output]:\n" << render_to_html(doctor) << "\n" << std::endl;
    std::cout << "\n[Qt Quick SceneGraph Output]:\n" << render_to_qml(doctor) << "\n" << std::endl;

    auto counter = Counter();
    std::cout << "\n[Reactive Counter HTML Output]:\n" << render_to_html(counter) << "\n" << std::endl;

    return 0;
}

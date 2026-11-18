// examples/reactive_counter/main.cpp
// PingDartUI Reactive Counter Example (PKG-138)
#include <pingdart/ui.h>
#include <iostream>

using namespace pingdart::ui;

Component Counter() {
    auto count = useState<int>(0);

    return Card({
        Heading("Reactive Counter", 2),
        Text([count] {
            return std::to_string(count.get());
        }).className("text-4xl font-bold").fontSize(32).color("#2563EB"),
        Row({
            Button("-").onClick([count]() mutable {
                count.set(count.get() - 1);
            }).padding(10, 20),
            Button("Reset").variant("secondary").onClick([count]() mutable {
                count.set(0);
            }).padding(10, 20),
            Button("+").variant("primary").onClick([count]() mutable {
                count.set(count.get() + 1);
            }).padding(10, 20)
        }).gap(12)
    }).padding(24).border(1, "#E2E8F0").borderRadius(12);
}

int main(int argc, char** argv) {
    std::cout << "==========================================" << std::endl;
    std::cout << " PingDartUI — Reactive Counter Application" << std::endl;
    std::cout << "==========================================" << std::endl;

    auto app = App(argc, argv)
        .title("Reactive Counter")
        .render(Counter);

    int res = app.run();
    std::cout << "[App Lifecycle]: Counter application initialized with code: " << res << std::endl;

    auto root = Counter();
    std::cout << "[Web SSR Output]:\n" << render_to_html(root) << std::endl;
    std::cout << "[Qt Quick Output]:\n" << render_to_qml(root) << std::endl;

    return 0;
}

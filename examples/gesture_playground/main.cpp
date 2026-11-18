// examples/gesture_playground/main.cpp
// PingDartUI Gesture Playground Example (PKG-138)
#include <pingdart/ui.h>
#include <iostream>

using namespace pingdart::ui;

Component GesturePlayground() {
    return Container({
        Heading("PingDartUI Gesture Playground", 1),
        Text("Multi-Touch & Pointer Gesture Recognition (Tap, Long Press, Swipe, Pan, Pinch)").color("#64748B"),
        Divider(),
        Row({
            Card({
                Heading("Tap & Long Press Card", 3),
                Text("Tap once or hold for 500ms for long-press.")
            })
            .padding(24).borderRadius(12).border(1, "#E2E8F0")
            .onTap([] {
                std::cout << "[Gesture]: Single Tap detected!" << std::endl;
            })
            .onLongPress([] {
                std::cout << "[Gesture]: Long Press detected (500ms held)!" << std::endl;
            }),

            Card({
                Heading("Swipe Card", 3),
                Text("Swipe left or right across this card.")
            })
            .padding(24).borderRadius(12).border(1, "#E2E8F0")
            .onSwipe([](Direction dir) {
                std::cout << "[Gesture]: Swipe detected in direction: " 
                          << (dir == Direction::Left ? "LEFT" : "RIGHT") << std::endl;
            }),

            Card({
                Heading("Pan & Drag Card", 3),
                Text("Click and drag across this card.")
            })
            .padding(24).borderRadius(12).border(1, "#E2E8F0")
            .onPan([](float dx, float dy) {
                std::cout << "[Gesture]: Pan delta: (" << dx << ", " << dy << ")" << std::endl;
            })
        }).gap(16)
    }).padding(24).gap(20);
}

int main(int argc, char** argv) {
    std::cout << "==========================================" << std::endl;
    std::cout << " PingDartUI — Gesture Playground Demo     " << std::endl;
    std::cout << "==========================================" << std::endl;

    auto app = App(argc, argv).title("Gesture Playground").render(GesturePlayground);
    app.run();

    auto card = GesturePlayground();
    std::cout << "[Web SSR Output generated: " << render_to_html(card).size() << " bytes]" << std::endl;
    std::cout << "[Qt Quick QML generated: " << render_to_qml(card).size() << " bytes]" << std::endl;

    return 0;
}

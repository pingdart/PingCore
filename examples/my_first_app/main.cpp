#include <pingcore/app.h>
#include <pingcore/ui.h>

using namespace pingcore;

int main() {
    App app("My First App");

    app.home(
        Column({
            Text("Welcome to PingCore")
                .fontSize(28)
                .bold(),

            Text("One codebase, multiple platforms"),

            Button("Get Started")
                .onClick([] {
                    print("Button clicked");
                })
        })
    );

    // Run single frame for verification
    app.singleFrame(true);

    int result = app.run();

    // Verify visual frame capture to file
    std::string ppm_path = "dist/first_app_frame.ppm";
    if (app.capture_frame_to_ppm(ppm_path)) {
        print("[Visual Verification] Framebuffer dumped to " + ppm_path);
    }

    print("[PingCore SDK] 'My First App' executed cleanly with return code: " + std::to_string(result));
    return result;
}

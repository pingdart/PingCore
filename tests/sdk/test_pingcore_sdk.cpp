// tests/sdk/test_pingcore_sdk.cpp
// PKG-118: Unit Tests for PingCore Developer Fluent SDK
#include <gtest/gtest.h>
#include <pingcore/app.h>
#include <pingcore/ui.h>
#include <filesystem>
#include <fstream>

using namespace pingcore;

TEST(PingCoreSdkGoogleTest, FluentWidgetHierarchyConstruction) {
    bool button_clicked = false;

    auto root = Column({
        Text("Welcome to PingCore")
            .fontSize(28.0f)
            .bold(true)
            .color(Color::rgb(255, 255, 255)),

        Text("One codebase, multiple platforms")
            .fontSize(16.0f),

        Button("Get Started")
            .onClick([&button_clicked] {
                button_clicked = true;
            })
    });

    auto widget_ptr = root.build();
    ASSERT_NE(widget_ptr, nullptr);
    EXPECT_NE(dynamic_cast<const pingdart::ui::Column*>(widget_ptr.get()), nullptr);
}

TEST(PingCoreSdkGoogleTest, AppSingleFrameLifecycleAndRender) {
    bool clicked = false;

    App app("SDK Test App");
    app.windowSize(640, 480);
    app.singleFrame(true);

    app.home(
        Column({
            Text("Test Text").fontSize(20.0f),
            Button("Click Me").onClick([&clicked] {
                clicked = true;
            })
        })
    );

    int rc = app.run();
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(app.width(), 640u);
    EXPECT_EQ(app.height(), 480u);
}

TEST(PingCoreSdkGoogleTest, FramebufferPpmImageCaptureVerification) {
    App app("Capture Test App");
    app.windowSize(320, 240);
    app.singleFrame(true);

    app.home(
        Container()
            .padding(16.0f)
            .background(Color::rgb(30, 41, 59))
            .child(
                Text("Render Verification").fontSize(18.0f).bold()
            )
    );

    int rc = app.run();
    EXPECT_EQ(rc, 0);

    std::string ppm_path = "dist/sdk_capture_test.ppm";
    std::filesystem::create_directories("dist");
    bool captured = app.capture_frame_to_ppm(ppm_path);
    if (captured) {
        EXPECT_TRUE(std::filesystem::exists(ppm_path));
        std::ifstream f(ppm_path, std::ios::binary);
        std::string header;
        f >> header;
        EXPECT_EQ(header, "P6");
        uint32_t w, h, max_val;
        f >> w >> h >> max_val;
        EXPECT_EQ(w, 320u);
        EXPECT_EQ(h, 240u);
        EXPECT_EQ(max_val, 255u);
        std::filesystem::remove(ppm_path);
    }
}

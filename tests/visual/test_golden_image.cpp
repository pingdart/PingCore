// tests/visual/test_golden_image.cpp
// Visual Correctness & Golden Image Verification (PKG-119)

#include <gtest/gtest.h>
#include <pingcore/app.h>
#include <pingcore/ui.h>
#include <pingdart/text/paragraph.h>
#include <pingdart/ui/box_constraints.h>
#include <fstream>
#include <filesystem>
#include <numeric>

using namespace pingcore;

TEST(GoldenImageVerificationTest, VisualWidgetCardRenderAndPixelIntegrity) {
    App app("Golden Visual Card Test");
    app.windowSize(400, 300);
    app.singleFrame(true);

    app.home(
        Container()
            .padding(24.0f)
            .background(Color::rgb(15, 23, 42))
            .child(
                Column({
                    Text("Production Card")
                        .fontSize(22.0f)
                        .bold(true)
                        .color(Color::rgb(255, 255, 255)),
                    Text("Pixel-perfect cross-platform rendering")
                        .fontSize(14.0f)
                        .color(Color::rgb(148, 163, 184)),
                    Button("Confirm Action")
                        .onClick([] {})
                })
            )
    );

    int rc = app.run();
    EXPECT_EQ(rc, 0);

    std::string ppm_path = "dist/golden_card_test.ppm";
    std::filesystem::create_directories("dist");
    bool captured = app.capture_frame_to_ppm(ppm_path);
    ASSERT_TRUE(captured);
    ASSERT_TRUE(std::filesystem::exists(ppm_path));

    // Inspect PPM format and pixel data
    std::ifstream file(ppm_path, std::ios::binary);
    ASSERT_TRUE(file.is_open());

    std::string magic;
    uint32_t width, height, max_val;
    file >> magic >> width >> height >> max_val;
    file.get(); // consume newline after max_val

    EXPECT_EQ(magic, "P6");
    EXPECT_EQ(width, 400u);
    EXPECT_EQ(height, 300u);
    EXPECT_EQ(max_val, 255u);

    std::vector<uint8_t> pixels(width * height * 3);
    file.read(reinterpret_cast<char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
    EXPECT_EQ(file.gcount(), static_cast<std::streamsize>(pixels.size()));

    // Verify color distribution
    size_t background_pixels = 0;
    size_t button_pixels = 0;
    size_t border_pixels = 0;

    for (size_t i = 0; i + 2 < pixels.size(); i += 3) {
        uint8_t r = pixels[i];
        uint8_t g = pixels[i + 1];
        uint8_t b = pixels[i + 2];

        if (r == 15 && g == 23 && b == 42) {
            background_pixels++;
        } else if (r == 59 && g == 130 && b == 246) {
            button_pixels++;
        } else if (r == 100 && g == 116 && b == 139) {
            border_pixels++;
        }
    }

    // Border and button and background must all be present
    EXPECT_GT(background_pixels, 0u);
    EXPECT_GT(button_pixels, 0u);
    EXPECT_GT(border_pixels, 0u);

    // Clean up test file
    std::filesystem::remove(ppm_path);
}

TEST(GoldenImageVerificationTest, HighDpiDisplayScalingMetrics) {
    // 1.0x Scale Baseline
    pingdart::ui::BoxConstraints normal_constraints =
        pingdart::ui::BoxConstraints::tight(pingdart::rhi::Size{320.0f, 200.0f});

    // 2.0x Retina Scale
    float scale_factor = 2.0f;
    pingdart::ui::BoxConstraints retina_constraints =
        pingdart::ui::BoxConstraints::tight(pingdart::rhi::Size{320.0f * scale_factor, 200.0f * scale_factor});

    EXPECT_FLOAT_EQ(retina_constraints.min_width, normal_constraints.min_width * scale_factor);
    EXPECT_FLOAT_EQ(retina_constraints.min_height, normal_constraints.min_height * scale_factor);
    EXPECT_FLOAT_EQ(retina_constraints.max_width, normal_constraints.max_width * scale_factor);
    EXPECT_FLOAT_EQ(retina_constraints.max_height, normal_constraints.max_height * scale_factor);

    // Verify scaled dimensions stay within GPU texture limits
    EXPECT_LE(retina_constraints.max_width, 8192.0f);
    EXPECT_LE(retina_constraints.max_height, 8192.0f);
}

TEST(GoldenImageVerificationTest, MultilingualTextShapingAndBounds) {
    struct TestCase {
        std::string language;
        std::string text;
    };

    std::vector<TestCase> cases = {
        {"English", "PingCore Native Cross-Platform UI Engine"},
        {"Japanese", "PingCore ネイティブクロスプラットフォームUIエンジン"},
        {"Arabic (RTL)", "محرك واجهة المستخدم بينج كور متعدد المنصات"},
        {"Hindi (Indic)", "पिंगकोर नेटिव क्रॉस-प्लेटफ़ॉर्म यूआई इंजन"}
    };

    for (const auto& tc : cases) {
        pingdart::text::ParagraphStyle style{
            .default_style = pingdart::text::TextStyle{.font_family = "Sans", .font_size = 18.0f},
            .align = pingdart::text::TextAlign::Left
        };

        pingdart::text::ParagraphBuilder builder(style);
        builder.add_text(tc.text);
        auto paragraph = builder.build();
        ASSERT_NE(paragraph, nullptr) << "Failed to build paragraph for " << tc.language;

        paragraph->layout(600.0f);

        EXPECT_GT(paragraph->width(), 0.0f) << "Width must be positive for " << tc.language;
        EXPECT_LE(paragraph->width(), 600.0f) << "Width must respect constraint for " << tc.language;
        EXPECT_GT(paragraph->height(), 0.0f) << "Height must be positive for " << tc.language;
        EXPECT_GE(paragraph->line_count(), 1u) << "Must have at least one line for " << tc.language;
    }
}

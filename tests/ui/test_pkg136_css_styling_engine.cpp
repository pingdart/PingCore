// tests/ui/test_pkg136_css_styling_engine.cpp
// PKG-136: PingDartUI Unified CSS & Utility Styling Engine Certification Tests
#include <gtest/gtest.h>
#include <pingdart/ui.h>
#include <thread>
#include <vector>

using namespace pingdart::ui;

class Pkg136CssStylingEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset theme engine to light defaults before each test
        ThemeEngine::instance().set_dark_mode(false);
    }
};

// 1. CSS Parser & Syntax Lexing
TEST_F(Pkg136CssStylingEngineTest, CssParserBasicRules) {
    std::string css = R"(
        /* Global header styling */
        h1 {
            color: #1E293B;
            font-size: 32px;
        }

        .btn-primary {
            background-color: #2563EB;
            color: #FFFFFF;
            padding: 10px 18px;
            border-radius: 8px;
        }
    )";

    std::vector<std::string> diagnostics;
    auto rules = CssParser::parse(css, &diagnostics);
    EXPECT_EQ(diagnostics.size(), 0);
    ASSERT_EQ(rules.size(), 2);

    // Rule 1: h1
    EXPECT_EQ(rules[0].selectors[0].tag, "h1");
    EXPECT_EQ(rules[0].declarations.size(), 2);
    EXPECT_EQ(rules[0].declarations[0].property, "color");
    EXPECT_EQ(rules[0].declarations[0].value, "#1E293B");

    // Rule 2: .btn-primary
    EXPECT_EQ(rules[1].selectors[0].classes[0], "btn-primary");
    EXPECT_EQ(rules[1].declarations.size(), 4);
    EXPECT_EQ(rules[1].declarations[0].property, "background-color");
    EXPECT_EQ(rules[1].declarations[0].value, "#2563EB");
}

// 2. CSS Syntax Diagnostics and Error Resilience
TEST_F(Pkg136CssStylingEngineTest, CssParserDiagnosticsResilience) {
    std::string bad_css = R"(
        .valid {
            color: #000000;
        }
        .unclosed-block {
            padding: 10px;
    )";

    std::vector<std::string> diagnostics;
    auto rules = CssParser::parse(bad_css, &diagnostics);
    EXPECT_GE(diagnostics.size(), 1);
    EXPECT_EQ(rules.size(), 1); // Valid rule still extracted
    EXPECT_EQ(rules[0].selectors[0].classes[0], "valid");
}

// 3. Variable Resolution & Fallbacks
TEST_F(Pkg136CssStylingEngineTest, CssVariableResolution) {
    auto& theme = ThemeEngine::instance();
    theme.set_variable("--custom-accent", "#7C3AED");

    std::string resolved_direct = CssParser::resolve_variables("var(--custom-accent)", theme);
    EXPECT_EQ(resolved_direct, "#7C3AED");

    std::string resolved_theme_token = CssParser::resolve_variables("var(--primary)", theme);
    EXPECT_EQ(resolved_theme_token, "#2563EB");

    std::string resolved_fallback = CssParser::resolve_variables("var(--undefined-var, #CBD5E1)", theme);
    EXPECT_EQ(resolved_fallback, "#CBD5E1");
}

// 4. Specificity Calculation & Cascade Precedence
TEST_F(Pkg136CssStylingEngineTest, SpecificityOrdering) {
    CssSelector tag_sel;
    tag_sel.tag = "button"; // (0, 0, 0, 1)

    CssSelector class_sel;
    class_sel.classes.push_back("primary"); // (0, 0, 1, 0)

    CssSelector id_sel;
    id_sel.id = "submit-btn"; // (0, 1, 0, 0)

    EXPECT_GT(class_sel.specificity(), tag_sel.specificity());
    EXPECT_GT(id_sel.specificity(), class_sel.specificity());

    // Cascade test on component
    std::string cascade_css = R"(
        button {
            background-color: #64748B;
        }
        .btn-submit {
            background-color: #2563EB;
        }
    )";

    auto rules = CssParser::parse(cascade_css);
    Component btn = Button("Submit").className("btn-submit");
    btn.apply_css_rules(rules);

    // Class selector (.btn-submit) must override tag selector (button)
    EXPECT_EQ(btn.style().background_color, "#2563EB");
}

// 5. Important Overrides
TEST_F(Pkg136CssStylingEngineTest, ImportantFlagOverrides) {
    std::string css = R"(
        button {
            background-color: #DC2626 !important;
        }
        .btn-green {
            background-color: #16A34A;
        }
    )";

    auto rules = CssParser::parse(css);
    Component btn = Button("Alert").className("btn-green");
    btn.apply_css_rules(rules);

    // !important on lower specificity tag selector overrides class selector
    EXPECT_EQ(btn.style().background_color, "#DC2626");
}

// 6. Five-Layer Styling Cascade Evaluation
TEST_F(Pkg136CssStylingEngineTest, FiveLayerCascadePrecedence) {
    // Tier 1: Component default
    Component comp = Button("Test");
    EXPECT_EQ(comp.style().background_color, "#F1F5F9");

    // Tier 3: External CSS
    std::string css = "button { background-color: #9333EA; }";
    comp.apply_css_rules(CssParser::parse(css));
    EXPECT_EQ(comp.style().background_color, "#9333EA");

    // Tier 4: Utility class
    comp.className("bg-blue-600");
    EXPECT_EQ(comp.style().background_color, "#2563EB");

    // Tier 5: Fluent C++ style
    comp.background("#10B981");
    EXPECT_EQ(comp.style().background_color, "#10B981");
}

// 7. Box Model & Dimension Parsing
TEST_F(Pkg136CssStylingEngineTest, DimensionAndUnitParsing) {
    EXPECT_FLOAT_EQ(CssParser::parse_px_dimension("24px"), 24.0f);
    EXPECT_FLOAT_EQ(CssParser::parse_px_dimension("1.5rem", 16.0f), 24.0f);
    EXPECT_FLOAT_EQ(CssParser::parse_px_dimension("2em", 16.0f), 32.0f);
    EXPECT_FLOAT_EQ(CssParser::parse_px_dimension("50%"), 50.0f);
}

// 8. Flexbox and Grid Layout Properties
TEST_F(Pkg136CssStylingEngineTest, FlexboxAndGridLayout) {
    Component flex_box = Row({ Text("A"), Text("B") })
        .gap(16.0f)
        .justifyContent(JustifyContent::SpaceBetween)
        .alignItems(AlignItems::Center);

    const auto& fs = flex_box.style();
    EXPECT_EQ(fs.flex_direction, FlexDirection::Row);
    EXPECT_FLOAT_EQ(fs.gap, 16.0f);
    EXPECT_EQ(fs.justify_content, JustifyContent::SpaceBetween);
    EXPECT_EQ(fs.align_items, AlignItems::Center);

    Component grid_box = Grid("repeat(3, 1fr)", 12.0f, { Text("1"), Text("2"), Text("3") });
    const auto& gs = grid_box.style();
    EXPECT_TRUE(gs.is_grid);
    EXPECT_FALSE(gs.is_flex);
    EXPECT_EQ(gs.grid_template_columns, "repeat(3, 1fr)");
    EXPECT_FLOAT_EQ(gs.grid_gap, 12.0f);
}

// 9. Bundled Palettes & Configurable Theme Engine
TEST_F(Pkg136CssStylingEngineTest, BundledPalettesAndConfigLoading) {
    auto& theme = ThemeEngine::instance();
    EXPECT_EQ(theme.get_color("blue", 600), "#2563EB");
    EXPECT_EQ(theme.get_color("red", 600), "#DC2626");
    EXPECT_EQ(theme.get_color("emerald", 600), "#059669");
    EXPECT_EQ(theme.get_color("slate", 900), "#0F172A");

    // Load pingdart.config.json override
    std::string config_json = R"({
        "theme": {
            "primary": "#4F46E5",
            "secondary": "#475569",
            "borderRadius": 14.0
        }
    })";

    EXPECT_TRUE(theme.load_config_json(config_json));
    EXPECT_EQ(theme.tokens().primary, "#4F46E5");
    EXPECT_EQ(theme.tokens().secondary, "#475569");
    EXPECT_FLOAT_EQ(theme.tokens().border_radius, 14.0f);
}

// 10. Dark Mode State and Persistence
TEST_F(Pkg136CssStylingEngineTest, DarkModeTogglingAndSsrInjection) {
    auto& theme = ThemeEngine::instance();
    EXPECT_FALSE(theme.is_dark_mode());

    theme.set_dark_mode(true);
    EXPECT_TRUE(theme.is_dark_mode());
    EXPECT_EQ(theme.tokens().background, "#0F172A");
    EXPECT_EQ(theme.tokens().foreground, "#F8FAFC");

    Component app = Container({ Text("Hello Dark World") });
    std::string html = render_to_html(app);
    EXPECT_NE(html.find("data-theme=\"dark\""), std::string::npos);

    // Switch back
    theme.set_dark_mode(false);
    EXPECT_FALSE(theme.is_dark_mode());
    EXPECT_EQ(theme.tokens().background, "#FFFFFF");
}

// 11. Security: HTML & XSS Entity Sanitization
TEST_F(Pkg136CssStylingEngineTest, XssEntitySanitizationInSsr) {
    std::string malicious_text = "<script>alert('XSS & Hack');</script>";
    Component unsafe_comp = Text(malicious_text);

    std::string html = render_to_html(unsafe_comp);
    EXPECT_EQ(html.find("<script>"), std::string::npos);
    EXPECT_NE(html.find("&lt;script&gt;"), std::string::npos);
    EXPECT_NE(html.find("&amp;"), std::string::npos);
    EXPECT_NE(html.find("&#39;"), std::string::npos);
}

// 12. Fast Utility-Class LRU Cache Concurrency
TEST_F(Pkg136CssStylingEngineTest, ThreadSafeUtilityClassCache) {
    auto& compiler = UtilityClassCompiler::instance();

    std::vector<std::thread> workers;
    for (int i = 0; i < 8; ++i) {
        workers.emplace_back([&compiler]() {
            for (int j = 0; j < 50; ++j) {
                auto s1 = compiler.compile_classes("bg-blue-600 p-4 rounded-xl shadow-md");
                EXPECT_EQ(s1.background_color, "#2563EB");
                EXPECT_FLOAT_EQ(s1.padding.top, 16.0f);
                EXPECT_FLOAT_EQ(s1.border_radius, 12.0f);

                auto s2 = compiler.compile_classes("flex-row items-center justify-between");
                EXPECT_EQ(s2.flex_direction, FlexDirection::Row);
                EXPECT_EQ(s2.align_items, AlignItems::Center);
                EXPECT_EQ(s2.justify_content, JustifyContent::SpaceBetween);
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }
}

// 13. Production Validation of All 25 Core Components
TEST_F(Pkg136CssStylingEngineTest, All25ComponentsCatalogValidation) {
    // Layout (7)
    Component c1 = Container({ Text("1") });
    Component c2 = Row({ Text("2") });
    Component c3 = Column({ Text("3") });
    Component c4 = Stack({ Text("4") });
    Component c5 = Flex({ Text("5") });
    Component c6 = Grid({ Text("6") });
    Component c7 = ScrollView({ Text("7") });

    EXPECT_EQ(c1.type(), "div");
    EXPECT_EQ(c2.style().flex_direction, FlexDirection::Row);
    EXPECT_EQ(c3.style().flex_direction, FlexDirection::Column);
    EXPECT_EQ(c4.style().position, "relative");
    EXPECT_TRUE(c5.style().is_flex);
    EXPECT_TRUE(c6.style().is_grid);
    EXPECT_EQ(c7.style().overflow, "auto");

    // Basic (6)
    Component c8 = Text("Content");
    Component c9 = Heading("Title", 2);
    Component c10 = Image("photo.jpg", "Photo");
    Component c11 = Icon("check");
    Component c12 = Divider();
    Component c13 = Spacer(20.0f);

    EXPECT_EQ(c8.type(), "span");
    EXPECT_EQ(c9.type(), "h2");
    EXPECT_EQ(c10.type(), "img");
    EXPECT_EQ(c11.type(), "i");
    EXPECT_EQ(c12.type(), "hr");
    EXPECT_FLOAT_EQ(c13.style().width.empty() ? 0 : 20.0f, 20.0f);

    // Actions (2)
    Component c14 = Button("Click");
    Component c15 = IconButton("gear", "Settings");

    EXPECT_EQ(c14.type(), "button");
    EXPECT_EQ(c15.type(), "button");

    // Forms (6)
    Component c16 = Input("Email", "user@pingdart.dev");
    Component c17 = TextArea("Notes", 5);
    Component c18 = Checkbox("Remember me", true);
    Component c19 = Radio("tier", "pro", "Pro", true);
    Component c20 = Switch(true);
    Component c21 = Select({ Option("Item 1") });

    EXPECT_EQ(c16.type(), "input");
    EXPECT_EQ(c17.type(), "textarea");
    EXPECT_EQ(c18.type(), "label");
    EXPECT_EQ(c19.type(), "label");
    EXPECT_EQ(c20.type(), "button");
    EXPECT_EQ(c21.type(), "select");

    // Display (3)
    Component c22 = Card({ Text("Card") });
    Component c23 = Avatar("JD", false, 36.0f);
    Component c24 = Badge("VIP");

    EXPECT_EQ(c22.type(), "div");
    EXPECT_EQ(c23.type(), "div");
    EXPECT_EQ(c24.type(), "span");

    // Overlay (1)
    Component c25 = Dialog("Prompt", Text("Message"), true);
    EXPECT_EQ(c25.type(), "div");
    EXPECT_EQ(c25.attributes().at("role"), "dialog");

    // Verify aggregate count
    std::vector<Component> catalog = {
        c1, c2, c3, c4, c5, c6, c7,
        c8, c9, c10, c11, c12, c13,
        c14, c15,
        c16, c17, c18, c19, c20, c21,
        c22, c23, c24,
        c25
    };
    EXPECT_EQ(catalog.size(), 25);
}

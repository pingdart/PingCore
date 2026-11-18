// tests/ui/test_pkg137_component_library.cpp
// PKG-137: Complete UI Component Library & Interactive Runtime Certification Tests
#include <gtest/gtest.h>
#include <pingdart/ui.h>
#include <string>
#include <vector>

using namespace pingdart::ui;

class Pkg137ComponentLibraryTest : public ::testing::Test {
protected:
    void SetUp() override {
        ThemeEngine::instance().set_dark_mode(false);
    }
};

// 1. Lexicographical Tuple Specificity Comparison (P0 Resolution)
TEST_F(Pkg137ComponentLibraryTest, LexicographicalTupleComparisonNoOverflow) {
    // ID selector (0, 1, 0, 0)
    CssSpecificity id_spec{0, 1, 0, 0};

    // Extreme class and element selector that would overflow a naive 100/10000 scalar formula
    CssSpecificity overflow_attempt{0, 0, 250, 500};

    // Lexicographical comparison: b=1 beats b=0 regardless of c or d
    EXPECT_TRUE(id_spec > overflow_attempt);
    EXPECT_FALSE(id_spec < overflow_attempt);
    EXPECT_FALSE(id_spec == overflow_attempt);

    // Inline style beats 1000 IDs
    CssSpecificity inline_spec{1, 0, 0, 0};
    CssSpecificity many_ids{0, 1000, 0, 0};
    EXPECT_TRUE(inline_spec > many_ids);

    // Same values evaluate equal
    CssSpecificity s1{0, 2, 3, 4};
    CssSpecificity s2{0, 2, 3, 4};
    EXPECT_TRUE(s1 == s2);
}

// 2. Functional QML Layout Properties and Bindings (P0 Resolution)
TEST_F(Pkg137ComponentLibraryTest, FunctionalQmlPropertiesAndBindings) {
    Component box = Container({
        Heading("Test Title"),
        Button("Click Me")
    }).width("100%").height("100%").borderRadius(12).gap(16);

    std::string qml = render_to_qml(box);

    // Verify properties are functional bindings, NOT emitted as comments
    EXPECT_NE(qml.find("width: parent.width"), std::string::npos);
    EXPECT_NE(qml.find("height: parent.height"), std::string::npos);
    EXPECT_NE(qml.find("radius: 12"), std::string::npos);
    EXPECT_NE(qml.find("spacing: 16"), std::string::npos);

    // Negative check: ensure layout properties are not commented out
    EXPECT_EQ(qml.find("// width: 100%"), std::string::npos);
    EXPECT_EQ(qml.find("// height: 100%"), std::string::npos);
    EXPECT_EQ(qml.find("// radius: 12"), std::string::npos);
}

// 3. Security: Context-Aware URL, CSS, and QML Sanitization (P0 Resolution)
TEST_F(Pkg137ComponentLibraryTest, ContextAwareSanitizationAndXssDefense) {
    // 3.1 URL Sanitizer
    EXPECT_EQ(sanitize_url("javascript:stealCookies()"), "#blocked-unsafe-url");
    EXPECT_EQ(sanitize_url("vbscript:runCommand()"), "#blocked-unsafe-url");
    EXPECT_EQ(sanitize_url("data:text/html;base64,PHNjcmlwdD4="), "#blocked-unsafe-url");
    EXPECT_EQ(sanitize_url("https://pingdart.dev/api/data"), "https://pingdart.dev/api/data");
    EXPECT_EQ(sanitize_url("/dashboard/patients"), "/dashboard/patients");

    // 3.2 QML String Escaper
    std::string unescaped = "Hello \"World\"\nNext Line\\Backslash";
    std::string escaped = escape_qml_string(unescaped);
    EXPECT_EQ(escaped.find("\\\""), 6);
    EXPECT_NE(escaped.find("\\n"), std::string::npos);
    EXPECT_NE(escaped.find("\\\\"), std::string::npos);

    // 3.3 CSS Value Sanitizer
    EXPECT_EQ(sanitize_css_value("expression(alert(1))"), "inherit");
    EXPECT_EQ(sanitize_css_value("behavior: url(script.htc)"), "inherit");
    EXPECT_EQ(sanitize_css_value("#2563EB"), "#2563EB");

    // 3.4 Web SSR Sanitized Output
    Component link = Link("Unsafe Link", "javascript:alert(1)");
    std::string html = render_to_html(link);
    EXPECT_EQ(html.find("javascript:"), std::string::npos);
    EXPECT_NE(html.find("#blocked-unsafe-url"), std::string::npos);
}

// 4. Scoped ThemeContext Independence (P1 Resolution)
TEST_F(Pkg137ComponentLibraryTest, ScopedThemeContextIndependence) {
    // Global theme is light by default
    auto& global_engine = ThemeEngine::instance();
    EXPECT_FALSE(global_engine.is_dark_mode());
    EXPECT_EQ(global_engine.tokens().primary, "#2563EB");

    // Create an independent window-level theme context
    auto window_theme = std::make_shared<ThemeContext>();
    window_theme->set_dark_mode(true);
    window_theme->tokens().primary = "#7C3AED"; // Violet 600

    EXPECT_TRUE(window_theme->is_dark_mode());
    EXPECT_EQ(window_theme->tokens().primary, "#7C3AED");

    // Global theme remains unaffected
    EXPECT_FALSE(global_engine.is_dark_mode());
    EXPECT_EQ(global_engine.tokens().primary, "#2563EB");

    // Attach to component
    Component card = Card({ Text("Card in Custom Theme") });
    card.theme(window_theme);
    EXPECT_EQ(card.theme()->tokens().primary, "#7C3AED");
}

// 5. Computed Layout Geometry Engine (P1 Resolution)
TEST_F(Pkg137ComponentLibraryTest, ComputedLayoutGeometryVerification) {
    Component row = Row({
        Text("Item 1").width("100px").height("40px"),
        Text("Item 2").width("150px").height("40px")
    }).padding(10, 20).gap(15).margin(5);

    // Compute layout for an 800x600 parent container
    row.compute_layout(0.0f, 0.0f, 800.0f, 600.0f);

    const auto& bounds = row.style().layout_bounds;
    EXPECT_FLOAT_EQ(bounds.x, 5.0f);
    EXPECT_FLOAT_EQ(bounds.y, 5.0f);

    // Children bounds
    const auto& c1_bounds = row.children()[0].style().layout_bounds;
    const auto& c2_bounds = row.children()[1].style().layout_bounds;

    // First child at x = row.x + padding.left = 5 + 20 = 25
    EXPECT_FLOAT_EQ(c1_bounds.x, 25.0f);
    EXPECT_FLOAT_EQ(c1_bounds.width, 100.0f);

    // Second child at x = c1.x + c1.w + gap = 25 + 100 + 15 = 140
    EXPECT_FLOAT_EQ(c2_bounds.x, 140.0f);
    EXPECT_FLOAT_EQ(c2_bounds.width, 150.0f);
}

// 6. Interactive Event Dispatching & Form Submission
TEST_F(Pkg137ComponentLibraryTest, InteractiveFormEventsAndSubmission) {
    bool submit_called = false;
    std::string captured_email;

    Component login_form = Form({
        Input("email", "doctor@hospital.org"),
        Input("password", "secret123")
    }).onSubmit([&](const FormData& data) {
        submit_called = true;
        captured_email = data.get("email");
    });

    FormData submission;
    submission.set("email", "doctor@hospital.org");
    submission.set("password", "secret123");

    login_form.emit_submit(submission);

    EXPECT_TRUE(submit_called);
    EXPECT_EQ(captured_email, "doctor@hospital.org");
}

// 7. DataGrid Fluent API & Structure
TEST_F(Pkg137ComponentLibraryTest, DataGridBuilderVerification) {
    std::vector<std::unordered_map<std::string, std::string>> sample_data = {
        {{"name", "Dr. Jane Smith"}, {"dept", "Cardiology"}},
        {{"name", "Dr. Raj Kumar"}, {"dept", "Neurology"}}
    };

    Component grid = DataGrid()
        .columns({
            ColumnDef("name", "Doctor Name"),
            ColumnDef("dept", "Department")
        })
        .data(sample_data)
        .searchable(true)
        .sortable(true)
        .pagination(10);

    EXPECT_EQ(grid.type(), "div");
    // Verify children contain search input, table, and pagination
    EXPECT_GE(grid.children().size(), 2);

    std::string html = render_to_html(grid);
    EXPECT_NE(html.find("Doctor Name"), std::string::npos);
    EXPECT_NE(html.find("Dr. Jane Smith"), std::string::npos);
    EXPECT_NE(html.find("Cardiology"), std::string::npos);
    EXPECT_NE(html.find("Search table records..."), std::string::npos);
}

// 8. Full 70-Component Complete Catalog Validation
TEST_F(Pkg137ComponentLibraryTest, Complete70ComponentCatalogValidation) {
    // 1. Layout (7)
    Component c1 = Container();
    Component c2 = Row();
    Component c3 = Column();
    Component c4 = Stack();
    Component c5 = Flex();
    Component c6 = Grid();
    Component c7 = ScrollView();

    // 2. Basic (6)
    Component c8 = Text("T");
    Component c9 = Heading("H", 1);
    Component c10 = Image("i.png");
    Component c11 = Icon("star");
    Component c12 = Divider();
    Component c13 = Spacer();

    // 3. Actions (4)
    Component c14 = Button("B");
    Component c15 = IconButton("gear");
    Component c16 = FloatingButton("plus");
    Component c17 = ButtonGroup({ Button("1"), Button("2") });

    // 4. Forms (7)
    Component c18 = Input();
    Component c19 = TextArea();
    Component c20 = Checkbox("C");
    Component c21 = Radio("r", "v", "L");
    Component c22 = Switch();
    Component c23 = Select({ Option("O") });
    Component c24 = Slider();

    // 5. Date & Time (3)
    Component c25 = DatePicker();
    Component c26 = TimePicker();
    Component c27 = Calendar();

    // 6. Navigation (6)
    Component c28 = Navbar({ Text("N") });
    Component c29 = Sidebar({ Text("S") });
    Component c30 = Tabs({ Tab("1", Text("C")) });
    Component c31 = Drawer({ Text("D") });
    Component c32 = Breadcrumb({"A", "B"});
    Component c33 = BottomNavigation({ Text("B") });

    // 7. Display (7)
    Component c34 = Card();
    Component c35 = Avatar("A");
    Component c36 = Badge("B");
    Component c37 = Table({ TableRow({ TableCell("C") }) });
    Component c38 = DataGrid();
    Component c39 = List({ ListItem("L") });
    Component c40 = Tree({ TreeNode("Root") });

    // 8. Feedback (6)
    Component c41 = Alert("A");
    Component c42 = Toast("T");
    Component c43 = Snackbar("S");
    Component c44 = ProgressBar(50);
    Component c45 = Spinner();
    Component c46 = Skeleton();

    // 9. Overlays (5)
    Component c47 = Dialog("D", Text("B"));
    Component c48 = Modal("M", Text("B"));
    Component c49 = Popover(Button("T"), Text("C"));
    Component c50 = Tooltip("T", Button("B"));
    Component c51 = BottomSheet({ Text("BS") });

    // 10. Application (5)
    Component c52 = AppShell({ Text("AS") });
    Component c53 = Page({ Text("P") });
    Component c54 = Outlet();
    Component c55 = SafeArea({ Text("SA") });
    Component c56 = StatCard("Stat", 100);

    // 11. Media & Advanced (7)
    Component c57 = VideoPlayer("v.mp4");
    Component c58 = AudioPlayer("a.mp3");
    Component c59 = ImageGallery({"1.jpg", "2.jpg"});
    Component c60 = Charts("Bar", {"A", "B"}, {10, 20});
    Component c61 = RichTextEditor();
    Component c62 = VirtualList(100, 30);
    Component c63 = FilePicker();

    // 12. Compound (7)
    Component c64 = Form({ Input() });
    Component c65 = FormField("Label", Input());
    Component c66 = SearchBox();
    Component c67 = Pagination(5, 1);
    Component c68 = Accordion({ Text("Section") });
    Component c69 = Carousel({ Text("Slide") });
    Component c70 = CommandPalette();

    std::vector<Component> all_components = {
        c1, c2, c3, c4, c5, c6, c7,
        c8, c9, c10, c11, c12, c13,
        c14, c15, c16, c17,
        c18, c19, c20, c21, c22, c23, c24,
        c25, c26, c27,
        c28, c29, c30, c31, c32, c33,
        c34, c35, c36, c37, c38, c39, c40,
        c41, c42, c43, c44, c45, c46,
        c47, c48, c49, c50, c51,
        c52, c53, c54, c55, c56,
        c57, c58, c59, c60, c61, c62, c63,
        c64, c65, c66, c67, c68, c69, c70
    };

    // Formally assert all 70 components exist, compile, and instantiate cleanly
    ASSERT_EQ(all_components.size(), 70);
    for (size_t i = 0; i < all_components.size(); ++i) {
        EXPECT_FALSE(all_components[i].type().empty()) << "Component index " << i << " has empty type tag";
    }
}

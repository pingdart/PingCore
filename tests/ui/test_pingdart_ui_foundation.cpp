// tests/ui/test_pingdart_ui_foundation.cpp
// PingDartUI Foundation Unit & Integration Test Suite (PKG-135)
#include <gtest/gtest.h>
#include <pingdart/ui.h>

using namespace pingdart::ui;

class PingDartUiFoundationTest : public ::testing::Test {
protected:
    void SetUp() override {
        Router::reset();
    }
};

// 1. Declarative Tree Structure
TEST_F(PingDartUiFoundationTest, DoctorCardDeclarativeTreeStructure) {
    Component card = Card({
        Heading("Dr. Jane Smith, MD"),
        Text("Cardiologist"),
        Text("15 years clinical experience"),
        Button("Book Consultation").variant("primary"),
        Link("View Full Profile", "/doctor/profile")
    }).className("doctor-card");

    EXPECT_EQ(card.type(), "div");
    EXPECT_EQ(card.children().size(), 5);
    EXPECT_EQ(card.children()[0].type(), "h1");
    EXPECT_EQ(card.children()[0].text(), "Dr. Jane Smith, MD");
    EXPECT_EQ(card.children()[1].type(), "span");
    EXPECT_EQ(card.children()[1].text(), "Cardiologist");
    EXPECT_EQ(card.children()[2].text(), "15 years clinical experience");
    EXPECT_EQ(card.children()[3].type(), "button");
    EXPECT_EQ(card.children()[3].text(), "Book Consultation");
    EXPECT_EQ(card.children()[4].type(), "a");
    EXPECT_EQ(card.children()[4].text(), "View Full Profile");
}

// 2. Three-Way Styling: Fluent Method Chaining
TEST_F(PingDartUiFoundationTest, FluentMethodChainingStyles) {
    Component btn = Button("Submit")
        .background("#2563EB")
        .color("#FFFFFF")
        .borderRadius(12)
        .padding(12, 24)
        .fontSize(18)
        .width("100%");

    const auto& s = btn.computed_style();
    EXPECT_EQ(s.background_color, "#2563EB");
    EXPECT_EQ(s.text_color, "#FFFFFF");
    EXPECT_FLOAT_EQ(s.border_radius, 12.0f);
    EXPECT_FLOAT_EQ(s.padding.top, 12.0f);
    EXPECT_FLOAT_EQ(s.padding.bottom, 12.0f);
    EXPECT_FLOAT_EQ(s.padding.left, 24.0f);
    EXPECT_FLOAT_EQ(s.padding.right, 24.0f);
    EXPECT_FLOAT_EQ(s.font_size, 18.0f);
    EXPECT_EQ(s.width, "100%");
}

// 3. Three-Way Styling: Tailwind Utility Class Lexer
TEST_F(PingDartUiFoundationTest, TailwindUtilityClassLexer) {
    Component box = Container({})
        .className("bg-blue-600 text-white rounded-xl p-4 gap-3 flex-row items-center justify-between shadow-md");

    const auto& s = box.computed_style();
    EXPECT_EQ(s.background_color, "#2563EB");
    EXPECT_EQ(s.text_color, "#FFFFFF");
    EXPECT_FLOAT_EQ(s.border_radius, 12.0f);
    EXPECT_FLOAT_EQ(s.padding.top, 16.0f);
    EXPECT_FLOAT_EQ(s.gap, 12.0f);
    EXPECT_EQ(s.flex_direction, FlexDirection::Row);
    EXPECT_EQ(s.align_items, AlignItems::Center);
    EXPECT_EQ(s.justify_content, JustifyContent::SpaceBetween);
    EXPECT_FLOAT_EQ(s.shadow.blur, 6.0f);
}

// 4. Reactive State Management
TEST_F(PingDartUiFoundationTest, ReactiveStateManagement) {
    auto count = useState<int>(0);
    EXPECT_EQ(count.get(), 0);

    bool callback_fired = false;
    count.subscribe([&](int val) {
        callback_fired = true;
        EXPECT_EQ(val, 42);
    });

    count.set(42);
    EXPECT_EQ(count.get(), 42);
    EXPECT_TRUE(callback_fired);
}

// 5. Reactive Dynamic Text Evaluation
TEST_F(PingDartUiFoundationTest, ReactiveDynamicTextEvaluation) {
    auto count = useState<int>(5);
    Component label = Text([count] {
        return "Count: " + std::to_string(count.get());
    });

    EXPECT_EQ(label.text(), "Count: 5");

    count.set(10);
    EXPECT_EQ(label.text(), "Count: 10");

    count.set(100);
    EXPECT_EQ(label.text(), "Count: 100");
}

// 6. Event Dispatching & Navigation Router
TEST_F(PingDartUiFoundationTest, EventDispatchingAndNavigationRouter) {
    bool clicked = false;
    Component btn = Button("Book Consultation")
        .onClick([&clicked] {
            clicked = true;
            Router::push("/booking/dr-smith");
        });

    EXPECT_FALSE(clicked);
    EXPECT_EQ(Router::current_path(), "/");

    btn.emit_click();

    EXPECT_TRUE(clicked);
    EXPECT_EQ(Router::current_path(), "/booking/dr-smith");

    Router::back();
    EXPECT_EQ(Router::current_path(), "/");
}

// 7. Web SSR HTML Output Generation
TEST_F(PingDartUiFoundationTest, WebSsrHtmlRendererOutput) {
    Component card = Card({
        Heading("Dr. Jane Smith, MD", 2),
        Text("Cardiologist")
    }).className("doctor-card");

    std::string html = render_to_html(card);
    EXPECT_NE(html.find("<div"), std::string::npos);
    EXPECT_NE(html.find("class=\"doctor-card\""), std::string::npos);
    EXPECT_NE(html.find("<h2"), std::string::npos);
    EXPECT_NE(html.find("Dr. Jane Smith, MD</h2>"), std::string::npos);
    EXPECT_NE(html.find("<span"), std::string::npos);
    EXPECT_NE(html.find("Cardiologist</span>"), std::string::npos);
    EXPECT_NE(html.find("</div>"), std::string::npos);
}

// 8. Qt Quick SceneGraph Output Generation
TEST_F(PingDartUiFoundationTest, QtQuickSceneGraphRendererOutput) {
    Component card = Card({
        Heading("Dr. Jane Smith, MD"),
        Button("Consult")
    });

    std::string qml = render_to_qml(card);
    EXPECT_NE(qml.find("import QtQuick"), std::string::npos);
    EXPECT_NE(qml.find("Column {"), std::string::npos);
    EXPECT_NE(qml.find("Text {"), std::string::npos);
    EXPECT_NE(qml.find("text: \"Dr. Jane Smith, MD\""), std::string::npos);
    EXPECT_NE(qml.find("Button {"), std::string::npos);
    EXPECT_NE(qml.find("text: \"Consult\""), std::string::npos);
}

// 9. App Container Pipeline
TEST_F(PingDartUiFoundationTest, AppContainerPipeline) {
    App app;
    app.stylesheet("styles/doctor.css")
       .render(Card({ Text("Test App") }));

    EXPECT_EQ(app.stylesheet_path(), "styles/doctor.css");
    EXPECT_EQ(app.root().type(), "div");
    EXPECT_EQ(app.root().children().size(), 1);
    EXPECT_EQ(app.root().children()[0].text(), "Test App");
}

// 10. Comprehensive Component Catalog Factories
TEST_F(PingDartUiFoundationTest, CoreComponentCatalogFactories) {
    EXPECT_EQ(Heading("Title", 1).type(), "h1");
    EXPECT_EQ(Heading("Title", 3).type(), "h3");
    EXPECT_EQ(Text("Plain").type(), "span");
    EXPECT_EQ(Button("Click").type(), "button");
    EXPECT_EQ(Link("Link", "/url").type(), "a");
    EXPECT_EQ(Input("Name").type(), "input");
    EXPECT_EQ(Image("avatar.png", "Avatar").type(), "img");
    EXPECT_EQ(Icon("heart").type(), "i");
    EXPECT_EQ(Badge("New").type(), "span");
    EXPECT_EQ(Alert("Warning").type(), "div");
    EXPECT_EQ(Divider().type(), "hr");
    EXPECT_EQ(Spacer(24).type(), "div");
    EXPECT_EQ(Row({}).type(), "div");
    EXPECT_EQ(Column({}).type(), "div");
}

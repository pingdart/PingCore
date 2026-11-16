#include <gtest/gtest.h>
#include <pingdart/web/semantic_widget.h>
#include <pingdart/web/deterministic_id.h>
#include "../../src/web/html/html_serializer.h"

using namespace pingdart::web;

TEST(SemanticWidgetsTest, DeterministicIdStabilityAndNesting) {
    // 1. Root without explicit key
    std::string root_id = make_deterministic_id("", 0);
    EXPECT_EQ(root_id, "pd_root");

    // 2. Child nodes without keys
    std::string child_0 = make_deterministic_id(root_id, 0);
    std::string child_1 = make_deterministic_id(root_id, 1);
    EXPECT_EQ(child_0, "pd_root_0");
    EXPECT_EQ(child_1, "pd_root_1");

    // 3. Child node with explicit key (hierarchically namespaced!)
    std::string card_id = make_deterministic_id(root_id, 2, "doctor_card");
    EXPECT_EQ(card_id, "pd_root_k_doctor_card");

    // 4. Grandchild under keyed node (no collision with another branch!)
    std::string button_id = make_deterministic_id(card_id, 0, "book_button");
    EXPECT_EQ(button_id, "pd_root_k_doctor_card_k_book_button");

    // 5. Repeatability: multiple calls produce identical result
    EXPECT_EQ(make_deterministic_id(card_id, 0, "book_button"), button_id);
}

TEST(SemanticWidgetsTest, SiblingKeyDuplicateDetection) {
    SiblingKeyTracker tracker;

    EXPECT_TRUE(tracker.track("doctor_card_1"));
    EXPECT_TRUE(tracker.track("doctor_card_2"));

    // Duplicate sibling key detected
    EXPECT_FALSE(tracker.track("doctor_card_1"));
}

TEST(SemanticWidgetsTest, SemanticHtmlTagsEmission) {
    HtmlSerializer serializer(false); // Compact mode for exact substring checks

    std::vector<std::unique_ptr<SemanticWidget>> main_children;
    main_children.push_back(std::make_unique<HeadingWidget>(1, "Find Doctors"));
    main_children.push_back(std::make_unique<HeadingWidget>(2, "Specialties"));
    main_children.push_back(std::make_unique<ParagraphWidget>("Browse top verified physicians."));
    main_children.push_back(std::make_unique<ButtonWidget>("Filter Results", "filter_action", "/doctors?filter=1", "btn"));
    main_children.push_back(std::make_unique<LinkWidget>("/doctors/all", "All Doctors"));
    main_children.push_back(std::make_unique<ImageWidget>("/img/banner.jpg", "Banner", 1200, 400));
    main_children.push_back(std::make_unique<TextFieldWidget>("doctor_name", "Search by name..."));

    MainWidget root(std::move(main_children), "main_page");

    render_semantic_tree(root, serializer);
    std::string html = serializer.to_string();

    // Verify actual semantic elements exist (not generic divs)
    EXPECT_NE(html.find("<main role=\"main\" data-pd-id=\"pd_root_k_main_page\">"), std::string::npos);
    EXPECT_NE(html.find("<h1 data-pd-id=\"pd_root_k_main_page_0\">Find Doctors</h1>"), std::string::npos);
    EXPECT_NE(html.find("<h2 data-pd-id=\"pd_root_k_main_page_1\">Specialties</h2>"), std::string::npos);
    EXPECT_NE(html.find("<p data-pd-id=\"pd_root_k_main_page_2\">Browse top verified physicians.</p>"), std::string::npos);
    EXPECT_NE(html.find("<form action=\"/doctors?filter=1\" method=\"POST\" style=\"display:inline;\"><button type=\"submit\" data-pd-id=\"pd_root_k_main_page_3\" data-pd-action=\"filter_action\" class=\"btn\">Filter Results</button></form>"), std::string::npos);
    EXPECT_NE(html.find("<a href=\"/doctors/all\" data-pd-id=\"pd_root_k_main_page_4\">All Doctors</a>"), std::string::npos);
    EXPECT_NE(html.find("<img src=\"/img/banner.jpg\" alt=\"Banner\" loading=\"lazy\" data-pd-id=\"pd_root_k_main_page_5\" width=\"1200\" height=\"400\" />"), std::string::npos);
    EXPECT_NE(html.find("<input type=\"text\" name=\"doctor_name\" data-pd-id=\"pd_root_k_main_page_6\" placeholder=\"Search by name...\" />"), std::string::npos);
    EXPECT_NE(html.find("</main>"), std::string::npos);
}

TEST(SemanticWidgetsTest, FlexboxLayoutEmission) {
    HtmlSerializer serializer(false);

    std::vector<std::unique_ptr<SemanticWidget>> col_children;
    col_children.push_back(std::make_unique<SpanWidget>("Item 1"));
    col_children.push_back(std::make_unique<SpanWidget>("Item 2"));

    FlexWidget flex_col(FlexDirection::Column, std::move(col_children), 16, "doctor-list");
    flex_col.render_html(serializer, "pd_col");

    std::string html = serializer.to_string();

    EXPECT_NE(html.find("style=\"display:flex;flex-direction:column;gap:16px;\""), std::string::npos);
    EXPECT_NE(html.find("class=\"doctor-list\""), std::string::npos);
    EXPECT_NE(html.find("<span data-pd-id=\"pd_col_0\">Item 1</span>"), std::string::npos);
    EXPECT_NE(html.find("<span data-pd-id=\"pd_col_1\">Item 2</span>"), std::string::npos);
}

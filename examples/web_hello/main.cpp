#include <pingdart/web/semantic_widget.h>
#include <pingdart/web/seo_metadata.h>
#include <pingdart/web/i_web_renderer.h>
#include "../../src/web/html/html_serializer.h"
#include "../../src/web/ssr/default_web_renderer.h"
#include <iostream>

using namespace pingdart::web;

int main() {
    // 1. Define Universal SEO Metadata
    SeoMetadata seo{
        .title = "Find Doctors | MeddyList",
        .description = "Browse verified physician and clinic profiles in your neighborhood.",
        .canonical_url = "https://meddylist.com/doctors",
        .locale = "en_US"
    };

    seo.open_graph = OpenGraphMetadata{
        .title = "Find Doctors | MeddyList",
        .description = "Browse verified physician and clinic profiles in your neighborhood.",
        .url = "https://meddylist.com/doctors",
        .image_url = "https://meddylist.com/og-doctors.png",
        .type = "website"
    };

    // Safe JSON-LD Schema (with verified script-breakout resistance)
    seo.json_ld_schemas.push_back(R"({
  "@context": "https://schema.org",
  "@type": "MedicalWebPage",
  "name": "Find Doctors",
  "about": "Physician search directory"
})");

    // 2. Build Semantic Widget Tree (Shared Application Model)
    std::vector<std::unique_ptr<SemanticWidget>> main_children;

    main_children.push_back(std::make_unique<HeadingWidget>(1, "Find Doctors Near You"));
    main_children.push_back(std::make_unique<ParagraphWidget>("Browse verified specialists and book appointments seamlessly."));

    // Doctor profile card
    std::vector<std::unique_ptr<SemanticWidget>> card_children;
    card_children.push_back(std::make_unique<HeadingWidget>(2, "Dr. Jane Smith, MD"));
    card_children.push_back(std::make_unique<ParagraphWidget>("Cardiologist · 15 years clinical experience"));
    card_children.push_back(std::make_unique<ButtonWidget>("Book Consultation", "book_consult_action", "/doctors/jane-smith/book", "btn-primary"));
    card_children.push_back(std::make_unique<LinkWidget>("/doctors/jane-smith", "View Full Profile"));

    main_children.push_back(std::make_unique<ArticleWidget>(std::move(card_children), "doctor-card", "doctor_card_1"));

    auto root = std::make_unique<MainWidget>(std::move(main_children), "main_root");

    // 3. Render Semantic HTML Body
    HtmlSerializer serializer(true);
    render_semantic_tree(*root, serializer);
    std::string body_html = serializer.to_string();

    // 4. Render Complete SSR Document
    DefaultWebRenderer renderer;
    std::string full_document = renderer.render_document(seo, body_html);

    std::cout << "=== PingDart Web SSR Output ===\n"
              << full_document
              << "===============================\n";

    return 0;
}

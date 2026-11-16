#include <gtest/gtest.h>
#include <pingdart/web/seo_metadata.h>

using namespace pingdart::web;

TEST(JsonLdSecurityTest, ScriptBreakoutResistance) {
    SeoMetadata seo;

    // Simulated attack payloads attempting </script> breakout and XSS
    std::string malicious_json = R"({
  "@context": "https://schema.org",
  "@type": "Physician",
  "name": "Dr. Test</script><script>alert('pwned')</script>",
  "description": "<!-- escape HTML comments -->",
  "comment": "Dangerous unicode \u2028 line separator"
})";

    seo.json_ld_schemas.push_back(malicious_json);

    std::string head_html = seo.render_head_tags();

    // 1. Must NOT contain unescaped </script> inside the script body
    // Search for occurrences of "</script>" - there should only be the closing tag of the JSON-LD script block itself
    size_t first_closing = head_html.find("</script>");
    ASSERT_NE(first_closing, std::string::npos);

    // After that, there should be no other </script> before the closing block
    size_t second_closing = head_html.find("</script>", first_closing + 1);
    EXPECT_EQ(second_closing, std::string::npos);

    // 2. The malicious tags must be escaped as unicode points
    EXPECT_NE(head_html.find("\\u003c/script\\u003e"), std::string::npos);
    EXPECT_NE(head_html.find("\\u003cscript\\u003e"), std::string::npos);
    EXPECT_NE(head_html.find("\\u003c!--"), std::string::npos);

    // 3. Raw '<' and '>' must not appear in the embedded JSON
    std::string sanitized = SeoMetadata::sanitize_json_for_html_script(malicious_json);
    EXPECT_EQ(sanitized.find('<'), std::string::npos);
    EXPECT_EQ(sanitized.find('>'), std::string::npos);
}

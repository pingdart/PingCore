#pragma once

#include <pingdart/web/i_html_serializer.h>
#include <string>
#include <vector>

namespace pingdart::web {

class HtmlSerializer final : public IHtmlSerializer {
public:
    explicit HtmlSerializer(bool pretty_print = true);
    ~HtmlSerializer() override = default;

    void write_doctype() override;
    void open_tag(std::string_view tag,
                  const std::vector<HtmlAttribute>& attributes = {},
                  bool self_closing = false) override;
    void close_tag(std::string_view tag) override;
    void write_text(std::string_view text, bool escape = true) override;
    void write_raw(std::string_view raw) override;
    [[nodiscard]] std::string to_string() const override;
    void reset() override;

    [[nodiscard]] static std::string escape_attribute(std::string_view value);
    [[nodiscard]] static std::string escape_text(std::string_view text);

private:
    void indent();

    bool pretty_print_{true};
    int indent_level_{0};
    std::string buffer_;
};

} // namespace pingdart::web

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace pingdart::web {

struct HtmlAttribute {
    std::string name;
    std::string value;
};

class IHtmlSerializer {
public:
    virtual ~IHtmlSerializer() = default;

    virtual void write_doctype() = 0;
    virtual void open_tag(std::string_view tag,
                          const std::vector<HtmlAttribute>& attributes = {},
                          bool self_closing = false) = 0;
    virtual void close_tag(std::string_view tag) = 0;
    virtual void write_text(std::string_view text, bool escape = true) = 0;
    virtual void write_raw(std::string_view raw) = 0;
    [[nodiscard]] virtual std::string to_string() const = 0;
    virtual void reset() = 0;
};

} // namespace pingdart::web

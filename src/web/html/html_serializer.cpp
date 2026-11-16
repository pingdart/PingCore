#include "html_serializer.h"

namespace pingdart::web {

HtmlSerializer::HtmlSerializer(bool pretty_print) : pretty_print_(pretty_print) {
    buffer_.reserve(2048);
}

std::string HtmlSerializer::escape_attribute(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char c : value) {
        switch (c) {
            case '&':  out.append("&amp;"); break;
            case '<':  out.append("&lt;"); break;
            case '>':  out.append("&gt;"); break;
            case '\"': out.append("&quot;"); break;
            case '\'': out.append("&#39;"); break;
            default:   out.push_back(c); break;
        }
    }
    return out;
}

std::string HtmlSerializer::escape_text(std::string_view text) {
    return escape_attribute(text);
}

void HtmlSerializer::indent() {
    if (!pretty_print_) return;
    for (int i = 0; i < indent_level_; ++i) {
        buffer_.append("  ");
    }
}

void HtmlSerializer::write_doctype() {
    buffer_.append("<!doctype html>");
    if (pretty_print_) {
        buffer_.push_back('\n');
    }
}

void HtmlSerializer::open_tag(std::string_view tag,
                             const std::vector<HtmlAttribute>& attributes,
                             bool self_closing) {
    indent();
    buffer_.push_back('<');
    buffer_.append(tag);

    for (const auto& attr : attributes) {
        buffer_.push_back(' ');
        buffer_.append(attr.name);
        buffer_.append("=\"");
        buffer_.append(escape_attribute(attr.value));
        buffer_.push_back('\"');
    }

    if (self_closing) {
        if (pretty_print_) {
            buffer_.append(" />\n");
        } else {
            buffer_.append(" />");
        }
    } else {
        if (pretty_print_) {
            buffer_.append(">\n");
        } else {
            buffer_.append(">");
        }
        indent_level_++;
    }
}

void HtmlSerializer::close_tag(std::string_view tag) {
    if (indent_level_ > 0) {
        indent_level_--;
    }
    indent();
    buffer_.append("</");
    buffer_.append(tag);
    if (pretty_print_) {
        buffer_.append(">\n");
    } else {
        buffer_.append(">");
    }
}

void HtmlSerializer::write_text(std::string_view text, bool escape) {
    indent();
    if (escape) {
        buffer_.append(escape_text(text));
    } else {
        buffer_.append(text);
    }
    if (pretty_print_) {
        buffer_.push_back('\n');
    }
}

void HtmlSerializer::write_raw(std::string_view raw) {
    buffer_.append(raw);
}

std::string HtmlSerializer::to_string() const {
    return buffer_;
}

void HtmlSerializer::reset() {
    buffer_.clear();
    indent_level_ = 0;
}

} // namespace pingdart::web

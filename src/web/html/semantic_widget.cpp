#include <pingdart/web/semantic_widget.h>

namespace pingdart::web {

void MainWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    serializer.open_tag("main", {
        {"role", "main"},
        {"data-pd-id", std::string(deterministic_id)}
    });

    for (size_t i = 0; i < children_.size(); ++i) {
        std::string child_id = make_deterministic_id(deterministic_id, i, children_[i]->key());
        children_[i]->render_html(serializer, child_id);
    }

    serializer.close_tag("main");
}

void ArticleWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    std::vector<HtmlAttribute> attrs{
        {"data-pd-id", std::string(deterministic_id)}
    };
    if (!css_class_.empty()) {
        attrs.push_back({"class", css_class_});
    }

    serializer.open_tag("article", attrs);

    for (size_t i = 0; i < children_.size(); ++i) {
        std::string child_id = make_deterministic_id(deterministic_id, i, children_[i]->key());
        children_[i]->render_html(serializer, child_id);
    }

    serializer.close_tag("article");
}

void NavWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    serializer.open_tag("nav", {
        {"data-pd-id", std::string(deterministic_id)}
    });

    for (size_t i = 0; i < children_.size(); ++i) {
        std::string child_id = make_deterministic_id(deterministic_id, i, children_[i]->key());
        children_[i]->render_html(serializer, child_id);
    }

    serializer.close_tag("nav");
}

void HeadingWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    std::string tag = "h" + std::to_string(level_);
    serializer.open_tag(tag, {
        {"data-pd-id", std::string(deterministic_id)}
    });
    serializer.write_text(text_);
    serializer.close_tag(tag);
}

void ParagraphWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    std::vector<HtmlAttribute> attrs{
        {"data-pd-id", std::string(deterministic_id)}
    };
    if (!css_class_.empty()) {
        attrs.push_back({"class", css_class_});
    }

    serializer.open_tag("p", attrs);
    serializer.write_text(text_);
    serializer.close_tag("p");
}

void SpanWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    std::vector<HtmlAttribute> attrs{
        {"data-pd-id", std::string(deterministic_id)}
    };
    if (!css_class_.empty()) {
        attrs.push_back({"class", css_class_});
    }

    serializer.open_tag("span", attrs);
    serializer.write_text(text_);
    serializer.close_tag("span");
}

void FormWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    std::vector<HtmlAttribute> attrs{
        {"action", action_},
        {"method", method_},
        {"data-pd-id", std::string(deterministic_id)}
    };
    if (!css_class_.empty()) {
        attrs.push_back({"class", css_class_});
    }

    serializer.open_tag("form", attrs);

    for (size_t i = 0; i < children_.size(); ++i) {
        std::string child_id = make_deterministic_id(deterministic_id, i, children_[i]->key());
        children_[i]->render_html(serializer, child_id);
    }

    serializer.close_tag("form");
}

void ButtonWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    const bool wrap_in_form = !fallback_href_.empty();

    if (wrap_in_form) {
        serializer.open_tag("form", {
            {"action", fallback_href_},
            {"method", "POST"},
            {"style", "display:inline;"}
        });
    }

    std::vector<HtmlAttribute> attrs{
        {"type", wrap_in_form ? "submit" : "button"},
        {"data-pd-id", std::string(deterministic_id)}
    };
    if (!action_id_.empty()) {
        attrs.push_back({"data-pd-action", action_id_});
    }
    if (!css_class_.empty()) {
        attrs.push_back({"class", css_class_});
    }

    serializer.open_tag("button", attrs);
    serializer.write_text(label_);
    serializer.close_tag("button");

    if (wrap_in_form) {
        serializer.close_tag("form");
    }
}

void LinkWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    std::vector<HtmlAttribute> attrs{
        {"href", href_},
        {"data-pd-id", std::string(deterministic_id)}
    };
    if (!css_class_.empty()) {
        attrs.push_back({"class", css_class_});
    }

    serializer.open_tag("a", attrs);
    serializer.write_text(label_);
    serializer.close_tag("a");
}

void ImageWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    std::vector<HtmlAttribute> attrs{
        {"src", src_},
        {"alt", alt_},
        {"loading", "lazy"},
        {"data-pd-id", std::string(deterministic_id)}
    };
    if (width_ > 0) {
        attrs.push_back({"width", std::to_string(width_)});
    }
    if (height_ > 0) {
        attrs.push_back({"height", std::to_string(height_)});
    }

    serializer.open_tag("img", attrs, /*self_closing=*/true);
}

void TextFieldWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    std::vector<HtmlAttribute> attrs{
        {"type", "text"},
        {"name", name_},
        {"data-pd-id", std::string(deterministic_id)}
    };
    if (!placeholder_.empty()) {
        attrs.push_back({"placeholder", placeholder_});
    }
    if (!value_.empty()) {
        attrs.push_back({"value", value_});
    }

    serializer.open_tag("input", attrs, /*self_closing=*/true);
}

void FlexWidget::render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const {
    std::string style = (direction_ == FlexDirection::Column)
        ? "display:flex;flex-direction:column;"
        : "display:flex;flex-direction:row;";
    if (gap_px_ > 0) {
        style += "gap:" + std::to_string(gap_px_) + "px;";
    }

    std::vector<HtmlAttribute> attrs{
        {"style", style},
        {"data-pd-id", std::string(deterministic_id)}
    };
    if (!css_class_.empty()) {
        attrs.push_back({"class", css_class_});
    }

    serializer.open_tag("div", attrs);

    for (size_t i = 0; i < children_.size(); ++i) {
        std::string child_id = make_deterministic_id(deterministic_id, i, children_[i]->key());
        children_[i]->render_html(serializer, child_id);
    }

    serializer.close_tag("div");
}

void render_semantic_tree(
    const SemanticWidget& root,
    IHtmlSerializer& serializer,
    std::string_view parent_id) {
    std::string id = make_deterministic_id(parent_id, 0, root.key());
    root.render_html(serializer, id);
}

} // namespace pingdart::web

#pragma once

#include "i_html_serializer.h"
#include "deterministic_id.h"
#include <string>
#include <vector>
#include <memory>
#include <string_view>

namespace pingdart::web {

class SemanticWidget {
public:
    explicit SemanticWidget(std::string key = "") : key_(std::move(key)) {}
    virtual ~SemanticWidget() = default;

    [[nodiscard]] std::string_view key() const noexcept { return key_; }

    virtual void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const = 0;

    [[nodiscard]] virtual size_t child_count() const noexcept { return 0; }
    [[nodiscard]] virtual const SemanticWidget* get_child(size_t index) const noexcept {
        (void)index;
        return nullptr;
    }

private:
    std::string key_;
};

// --- Structural & Content Widgets ---

class MainWidget final : public SemanticWidget {
public:
    explicit MainWidget(std::vector<std::unique_ptr<SemanticWidget>> children, std::string key = "")
        : SemanticWidget(std::move(key)), children_(std::move(children)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;
    [[nodiscard]] size_t child_count() const noexcept override { return children_.size(); }
    [[nodiscard]] const SemanticWidget* get_child(size_t index) const noexcept override {
        return index < children_.size() ? children_[index].get() : nullptr;
    }

private:
    std::vector<std::unique_ptr<SemanticWidget>> children_;
};

class ArticleWidget final : public SemanticWidget {
public:
    explicit ArticleWidget(std::vector<std::unique_ptr<SemanticWidget>> children, std::string css_class = "", std::string key = "")
        : SemanticWidget(std::move(key)), children_(std::move(children)), css_class_(std::move(css_class)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;
    [[nodiscard]] size_t child_count() const noexcept override { return children_.size(); }
    [[nodiscard]] const SemanticWidget* get_child(size_t index) const noexcept override {
        return index < children_.size() ? children_[index].get() : nullptr;
    }

private:
    std::vector<std::unique_ptr<SemanticWidget>> children_;
    std::string css_class_;
};

class NavWidget final : public SemanticWidget {
public:
    explicit NavWidget(std::vector<std::unique_ptr<SemanticWidget>> children, std::string key = "")
        : SemanticWidget(std::move(key)), children_(std::move(children)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;
    [[nodiscard]] size_t child_count() const noexcept override { return children_.size(); }
    [[nodiscard]] const SemanticWidget* get_child(size_t index) const noexcept override {
        return index < children_.size() ? children_[index].get() : nullptr;
    }

private:
    std::vector<std::unique_ptr<SemanticWidget>> children_;
};

class HeadingWidget final : public SemanticWidget {
public:
    HeadingWidget(uint8_t level, std::string text, std::string key = "")
        : SemanticWidget(std::move(key)),
          level_(level >= 1 && level <= 6 ? level : 1),
          text_(std::move(text)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;

private:
    uint8_t level_;
    std::string text_;
};

class ParagraphWidget final : public SemanticWidget {
public:
    explicit ParagraphWidget(std::string text, std::string css_class = "", std::string key = "")
        : SemanticWidget(std::move(key)), text_(std::move(text)), css_class_(std::move(css_class)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;

private:
    std::string text_;
    std::string css_class_;
};

class SpanWidget final : public SemanticWidget {
public:
    explicit SpanWidget(std::string text, std::string css_class = "", std::string key = "")
        : SemanticWidget(std::move(key)), text_(std::move(text)), css_class_(std::move(css_class)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;

private:
    std::string text_;
    std::string css_class_;
};

class FormWidget final : public SemanticWidget {
public:
    FormWidget(std::string action,
               std::string method,
               std::vector<std::unique_ptr<SemanticWidget>> children,
               std::string css_class = "",
               std::string key = "")
        : SemanticWidget(std::move(key)),
          action_(std::move(action)),
          method_(std::move(method)),
          children_(std::move(children)),
          css_class_(std::move(css_class)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;
    [[nodiscard]] size_t child_count() const noexcept override { return children_.size(); }
    [[nodiscard]] const SemanticWidget* get_child(size_t index) const noexcept override {
        return index < children_.size() ? children_[index].get() : nullptr;
    }
    [[nodiscard]] std::string_view action() const noexcept { return action_; }
    [[nodiscard]] std::string_view method() const noexcept { return method_; }

private:
    std::string action_;
    std::string method_{"GET"};
    std::vector<std::unique_ptr<SemanticWidget>> children_;
    std::string css_class_;
};

class ButtonWidget final : public SemanticWidget {
public:
    explicit ButtonWidget(std::string label,
                          std::string action_id = "",
                          std::string fallback_href = "",
                          std::string css_class = "",
                          std::string key = "")
        : SemanticWidget(std::move(key)),
          label_(std::move(label)),
          action_id_(std::move(action_id)),
          fallback_href_(std::move(fallback_href)),
          css_class_(std::move(css_class)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;
    [[nodiscard]] std::string_view fallback_href() const noexcept { return fallback_href_; }

private:
    std::string label_;
    std::string action_id_;
    std::string fallback_href_;
    std::string css_class_;
};

class LinkWidget final : public SemanticWidget {
public:
    LinkWidget(std::string href, std::string label, std::string css_class = "", std::string key = "")
        : SemanticWidget(std::move(key)),
          href_(std::move(href)),
          label_(std::move(label)),
          css_class_(std::move(css_class)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;

private:
    std::string href_;
    std::string label_;
    std::string css_class_;
};

class ImageWidget final : public SemanticWidget {
public:
    ImageWidget(std::string src, std::string alt, uint32_t width = 0, uint32_t height = 0, std::string key = "")
        : SemanticWidget(std::move(key)),
          src_(std::move(src)),
          alt_(std::move(alt)),
          width_(width),
          height_(height) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;

private:
    std::string src_;
    std::string alt_;
    uint32_t width_;
    uint32_t height_;
};

class TextFieldWidget final : public SemanticWidget {
public:
    TextFieldWidget(std::string name, std::string placeholder = "", std::string value = "", std::string key = "")
        : SemanticWidget(std::move(key)),
          name_(std::move(name)),
          placeholder_(std::move(placeholder)),
          value_(std::move(value)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;

private:
    std::string name_;
    std::string placeholder_;
    std::string value_;
};

enum class FlexDirection : uint8_t {
    Column,
    Row
};

class FlexWidget final : public SemanticWidget {
public:
    FlexWidget(FlexDirection direction,
               std::vector<std::unique_ptr<SemanticWidget>> children,
               uint32_t gap_px = 0,
               std::string css_class = "",
               std::string key = "")
        : SemanticWidget(std::move(key)),
          direction_(direction),
          children_(std::move(children)),
          gap_px_(gap_px),
          css_class_(std::move(css_class)) {}

    void render_html(IHtmlSerializer& serializer, std::string_view deterministic_id) const override;
    [[nodiscard]] size_t child_count() const noexcept override { return children_.size(); }
    [[nodiscard]] const SemanticWidget* get_child(size_t index) const noexcept override {
        return index < children_.size() ? children_[index].get() : nullptr;
    }

private:
    FlexDirection direction_;
    std::vector<std::unique_ptr<SemanticWidget>> children_;
    uint32_t gap_px_;
    std::string css_class_;
};

// Renders the entire semantic widget tree with recursive deterministic IDs
void render_semantic_tree(
    const SemanticWidget& root,
    IHtmlSerializer& serializer,
    std::string_view parent_id = "");

} // namespace pingdart::web

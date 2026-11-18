// include/pingcore/ui.h
// PingCore Simplified High-Level Developer UI SDK (PKG-118)
#pragma once

#include <pingdart/ui/widget.h>
#include <pingdart/ui/layout_primitives.h>
#include <pingdart/ui/leaf_widgets.h>
#include <pingdart/ui/focus_node.h>
#include <pingdart/ui/text_editing_controller.h>
#include <pingdart/web/url_security.h>
#include <pingcore/reactive.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace pingcore {

// General Widget handle
using Widget = std::shared_ptr<const pingdart::ui::Widget>;

// Forward declaration of navigation helper
void navigate(std::string_view path);

// Global console helper
inline void print(std::string_view msg) {
    std::cout << msg << "\n";
}

// Fluent Color Type
struct Color {
    uint8_t a{255};
    uint8_t r{255};
    uint8_t g{255};
    uint8_t b{255};

    static Color rgb(uint8_t r, uint8_t g, uint8_t b) {
        return Color{255, r, g, b};
    }
    static Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return Color{a, r, g, b};
    }

    [[nodiscard]] pingdart::ui::Color to_native() const noexcept {
        return pingdart::ui::Color::from_argb(a, r, g, b);
    }
};

// --- Fluent Text Widget ---
class Text {
public:
    explicit Text(std::string_view content)
        : text_(content) {}

    Text(std::function<std::string()> reactive_evaluator)
        : reactive_evaluator_(std::move(reactive_evaluator)) {
        if (reactive_evaluator_) {
            text_ = reactive_evaluator_();
        }
    }

    [[nodiscard]] std::string current_text() const {
        if (reactive_evaluator_) {
            return reactive_evaluator_();
        }
        return text_;
    }

    [[nodiscard]] bool is_reactive() const noexcept {
        return reactive_evaluator_ != nullptr;
    }

    Text& fontSize(float size) {
        size_ = size;
        return *this;
    }

    Text& bold(bool is_bold = true) {
        bold_ = is_bold;
        return *this;
    }

    Text& color(Color c) {
        color_ = c;
        return *this;
    }

    Text& fontFamily(std::string_view family) {
        family_ = family;
        return *this;
    }

    [[nodiscard]] std::shared_ptr<const pingdart::ui::Widget> build() const {
        return std::make_shared<pingdart::ui::Text>(
            text_,
            size_,
            color_.to_native(),
            family_,
            bold_
        );
    }

    operator std::shared_ptr<const pingdart::ui::Widget>() const {
        return build();
    }

private:
    std::string text_;
    std::function<std::string()> reactive_evaluator_{nullptr};
    float size_{16.0f};
    bool bold_{false};
    Color color_{Color::rgb(240, 240, 240)};
    std::string family_{"sans-serif"};
};

// --- Fluent Button Widget ---
class Button {
public:
    explicit Button(std::string_view label)
        : label_(label) {}

    Button& onClick(std::function<void()> on_click) {
        on_click_ = std::move(on_click);
        return *this;
    }

    Button& navigateTo(std::string_view path) {
        std::string decoded_path;
        if (path.empty() || path.front() != '/' ||
            !pingdart::web::UrlSecurity::is_safe_uri_path(path) ||
            !pingdart::web::UrlSecurity::percent_decode(path, decoded_path) ||
            decoded_path.starts_with("//") || decoded_path.find('\\') != std::string::npos) {
            throw std::invalid_argument("Button navigation target must be a safe absolute route path");
        }
        navigation_target_ = path;
        on_click_ = [target = navigation_target_] {
            navigate(target);
        };
        return *this;
    }

    [[nodiscard]] std::shared_ptr<const pingdart::ui::Widget> build() const {
        return std::make_shared<pingdart::ui::Button>(
            label_,
            on_click_,
            nullptr,
            pingdart::ui::Color::from_argb(255, 37, 99, 235),
            pingdart::ui::EdgeInsets::symmetric(10.0f, 16.0f),
            6.0f,
            nullptr,
            navigation_target_
        );
    }

    operator std::shared_ptr<const pingdart::ui::Widget>() const {
        return build();
    }

private:
    std::string label_;
    std::function<void()> on_click_;
    std::string navigation_target_;
};

// --- Fluent Column Layout ---
class Column {
public:
    Column(std::initializer_list<std::shared_ptr<const pingdart::ui::Widget>> children)
        : children_(children) {}

    explicit Column(std::vector<std::shared_ptr<const pingdart::ui::Widget>> children)
        : children_(std::move(children)) {}

    Column& spacing(float gap) {
        gap_ = gap;
        return *this;
    }

    [[nodiscard]] std::shared_ptr<const pingdart::ui::Widget> build() const {
        if (gap_ <= 0.0f) {
            return std::make_shared<pingdart::ui::Column>(
                children_,
                pingdart::ui::MainAxisAlignment::Start,
                pingdart::ui::CrossAxisAlignment::Stretch
            );
        }

        std::vector<std::shared_ptr<const pingdart::ui::Widget>> spaced_children;
        spaced_children.reserve(children_.size() * 2);
        for (size_t i = 0; i < children_.size(); ++i) {
            spaced_children.push_back(children_[i]);
            if (i + 1 < children_.size()) {
                spaced_children.push_back(std::make_shared<pingdart::ui::SizedBox>(0.0f, gap_));
            }
        }
        return std::make_shared<pingdart::ui::Column>(
            spaced_children,
            pingdart::ui::MainAxisAlignment::Start,
            pingdart::ui::CrossAxisAlignment::Stretch
        );
    }

    operator std::shared_ptr<const pingdart::ui::Widget>() const {
        return build();
    }

private:
    std::vector<std::shared_ptr<const pingdart::ui::Widget>> children_;
    float gap_{12.0f};
};

// --- Fluent Row Layout ---
class Row {
public:
    Row(std::initializer_list<std::shared_ptr<const pingdart::ui::Widget>> children)
        : children_(children) {}

    explicit Row(std::vector<std::shared_ptr<const pingdart::ui::Widget>> children)
        : children_(std::move(children)) {}

    [[nodiscard]] std::shared_ptr<const pingdart::ui::Widget> build() const {
        return std::make_shared<pingdart::ui::Row>(
            children_,
            pingdart::ui::MainAxisAlignment::Start,
            pingdart::ui::CrossAxisAlignment::Center
        );
    }

    operator std::shared_ptr<const pingdart::ui::Widget>() const {
        return build();
    }

private:
    std::vector<std::shared_ptr<const pingdart::ui::Widget>> children_;
};

// --- Fluent Center Widget ---
class Center {
public:
    explicit Center(std::shared_ptr<const pingdart::ui::Widget> child)
        : child_(std::move(child)) {}

    [[nodiscard]] std::shared_ptr<const pingdart::ui::Widget> build() const {
        return std::make_shared<pingdart::ui::Center>(child_);
    }

    operator std::shared_ptr<const pingdart::ui::Widget>() const {
        return build();
    }

private:
    std::shared_ptr<const pingdart::ui::Widget> child_;
};

// --- Fluent Container Widget ---
class Container {
public:
    Container() = default;

    Container& child(std::shared_ptr<const pingdart::ui::Widget> c) {
        child_ = std::move(c);
        return *this;
    }

    Container& width(float w) {
        width_ = w;
        return *this;
    }

    Container& height(float h) {
        height_ = h;
        return *this;
    }

    Container& padding(float all) {
        padding_ = pingdart::ui::EdgeInsets::all(all);
        return *this;
    }

    Container& background(Color c) {
        bg_ = c;
        return *this;
    }

    Container& borderRadius(float r) {
        radius_ = r;
        return *this;
    }

    [[nodiscard]] std::shared_ptr<const pingdart::ui::Widget> build() const {
        return std::make_shared<pingdart::ui::Container>(
            width_,
            height_,
            bg_.to_native(),
            padding_,
            radius_,
            child_
        );
    }

    operator std::shared_ptr<const pingdart::ui::Widget>() const {
        return build();
    }

private:
    std::shared_ptr<const pingdart::ui::Widget> child_;
    float width_{pingdart::ui::BoxConstraints::infinity};
    float height_{pingdart::ui::BoxConstraints::infinity};
    pingdart::ui::EdgeInsets padding_{pingdart::ui::EdgeInsets::zero()};
    Color bg_{Color::rgba(0, 0, 0, 0)};
    float radius_{0.0f};
};

// --- Fluent TextField Widget ---
class TextField {
public:
    explicit TextField(std::string_view placeholder = "")
        : placeholder_(placeholder) {}

    TextField& value(std::string_view val) {
        value_ = val;
        return *this;
    }

    TextField& onChanged(std::function<void(std::string_view)> cb) {
        on_changed_ = std::move(cb);
        return *this;
    }

    [[nodiscard]] std::shared_ptr<const pingdart::ui::Widget> build() const {
        std::function<void(const std::string&)> fn = nullptr;
        if (on_changed_) {
            fn = [cb = on_changed_](const std::string& s) { cb(s); };
        }
        return std::make_shared<pingdart::ui::TextField>(
            placeholder_,
            value_,
            std::move(fn)
        );
    }

    operator std::shared_ptr<const pingdart::ui::Widget>() const {
        return build();
    }

private:
    std::string placeholder_;
    std::string value_;
    std::function<void(std::string_view)> on_changed_;
};

// --- Fluent PasswordField Widget ---
class PasswordField {
public:
    explicit PasswordField(std::string_view placeholder = "Password")
        : placeholder_(placeholder) {}

    PasswordField& onChanged(std::function<void(std::string_view)> cb) {
        on_changed_ = std::move(cb);
        return *this;
    }

    [[nodiscard]] std::shared_ptr<const pingdart::ui::Widget> build() const {
        std::function<void(const std::string&)> fn = nullptr;
        if (on_changed_) {
            fn = [cb = on_changed_](const std::string& s) { cb(s); };
        }
        return std::make_shared<pingdart::ui::TextField>(
            nullptr,
            nullptr,
            placeholder_,
            true,
            std::move(fn)
        );
    }

    operator std::shared_ptr<const pingdart::ui::Widget>() const {
        return build();
    }

private:
    std::string placeholder_;
    std::function<void(std::string_view)> on_changed_;
};

} // namespace pingcore

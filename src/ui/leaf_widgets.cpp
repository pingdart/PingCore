#include <pingdart/ui/leaf_widgets.h>
#include <algorithm>

namespace pingdart::ui {

// RenderParagraphBox
RenderParagraphBox::RenderParagraphBox(std::string text,
                                       std::string font_family,
                                       float font_size,
                                       Color color,
                                       bool bold,
                                       bool italic)
    : text_(std::move(text)),
      font_family_(std::move(font_family)),
      font_size_(font_size),
      color_(color),
      bold_(bold),
      italic_(italic) {}

void RenderParagraphBox::set_text(std::string text) {
    if (text_ == text) return;
    text_ = std::move(text);
    mark_needs_layout();
}

void RenderParagraphBox::set_font_size(float size) {
    if (font_size_ == size) return;
    font_size_ = size;
    mark_needs_layout();
}

void RenderParagraphBox::set_color(Color c) {
    if (color_ == c) return;
    color_ = c;
    mark_needs_paint();
}

void RenderParagraphBox::perform_layout() {
    float estimated_width = static_cast<float>(text_.length()) * (font_size_ * 0.55f);
    float estimated_height = font_size_ * 1.25f;
    size_ = constraints_.constrain(Size{estimated_width, estimated_height});
}

void RenderParagraphBox::paint(PaintContext& context, Offset offset) {
    if (text_.empty()) return;

    pingdart::rhi::TextBlob blob;
    blob.text = text_;
    blob.font_family = font_family_;
    blob.font_size = font_size_;
    blob.bold = bold_;
    blob.italic = italic_;

    pingdart::rhi::Paint paint;
    paint.color = color_;
    paint.style = pingdart::rhi::PaintStyle::Fill;

    context.builder().draw_text_blob(blob, offset.to_point(), paint);
}

// Text Widget
std::unique_ptr<RenderObject> Text::create_render_object(BuildContext& context) const {
    (void)context;
    return std::make_unique<RenderParagraphBox>(
        text_, font_family_, font_size_, color_, bold_, italic_);
}

void Text::update_render_object(BuildContext& context, RenderObject& render_object) const {
    (void)context;
    auto* rpb = dynamic_cast<RenderParagraphBox*>(&render_object);
    if (rpb) {
        rpb->set_text(text_);
        rpb->set_font_size(font_size_);
        rpb->set_color(color_);
    }
}

// Button Implementation
Button::Button(std::shared_ptr<const Widget> child,
               std::function<void()> on_pressed,
               std::shared_ptr<FocusNode> focus_node,
               Color background_color,
               EdgeInsets padding,
               float border_radius,
               std::shared_ptr<const Key> key)
    : StatefulWidget(std::move(key)),
      child_(std::move(child)),
      on_pressed_(std::move(on_pressed)),
      focus_node_(std::move(focus_node)),
      background_color_(background_color),
      padding_(padding),
      border_radius_(border_radius) {}

Button::Button(std::string text,
               std::function<void()> on_pressed,
               std::shared_ptr<FocusNode> focus_node,
               Color background_color,
               EdgeInsets padding,
               float border_radius,
               std::shared_ptr<const Key> key)
    : Button(std::make_shared<Text>(std::move(text), 14.0f, Color::from_argb(255, 255, 255, 255), "sans-serif", true),
             std::move(on_pressed),
             std::move(focus_node),
             background_color,
             padding,
             border_radius,
             std::move(key)) {}

class ButtonState : public State<Button> {
public:
    void init_state() override {
        effective_focus_ = widget().focus_node();
        if (!effective_focus_) {
            effective_focus_ = std::make_shared<FocusNode>("Button");
        }
        effective_focus_->add_listener([this](bool /*focused*/) {
            set_state([]() {});
        });
        effective_focus_->set_on_key([this](const pal::KeyboardEvent& ev) {
            if (ev.type == pal::EventType::KeyDown &&
                (ev.virtual_key == 13 || ev.virtual_key == 10 || ev.virtual_key == 32 || ev.key_code == 0xFF0D)) {
                if (widget().on_pressed()) {
                    widget().on_pressed()();
                    return true;
                }
            }
            return false;
        });
    }

    std::shared_ptr<Widget> build(BuildContext& /*context*/) override {
        const auto& w = widget();
        Color bg = w.background_color();
        if (effective_focus_ && effective_focus_->has_focus()) {
            bg = Color::from_rgba_f(std::min(1.0f, bg.r + 0.12f),
                                    std::min(1.0f, bg.g + 0.12f),
                                    std::min(1.0f, bg.b + 0.15f),
                                    bg.a);
        }

        auto inner_container = std::make_shared<Container>(
            BoxConstraints::infinity,
            BoxConstraints::infinity,
            bg,
            w.padding(),
            w.border_radius(),
            w.child()
        );

        return std::make_shared<GestureDetector>(
            inner_container,
            [this]() {
                if (effective_focus_) {
                    effective_focus_->request_focus();
                }
                if (widget().on_pressed()) {
                    widget().on_pressed()();
                }
            }
        );
    }

private:
    std::shared_ptr<FocusNode> effective_focus_;
};

std::unique_ptr<StateBase> Button::create_state() const {
    return std::make_unique<ButtonState>();
}

// TextField Implementation
TextField::TextField(std::shared_ptr<TextEditingController> controller,
                     std::shared_ptr<FocusNode> focus_node,
                     std::string placeholder,
                     bool obscure_text,
                     std::function<void(const std::string&)> on_changed,
                     std::function<void(const std::string&)> on_submitted,
                     std::shared_ptr<const Key> key)
    : StatefulWidget(std::move(key)),
      controller_(std::move(controller)),
      focus_node_(std::move(focus_node)),
      placeholder_(std::move(placeholder)),
      obscure_text_(obscure_text),
      on_changed_(std::move(on_changed)),
      on_submitted_(std::move(on_submitted)) {}

TextField::TextField(std::string placeholder,
                     std::string text,
                     std::function<void(const std::string&)> on_changed,
                     std::shared_ptr<const Key> key)
    : TextField(std::make_shared<TextEditingController>(std::move(text)),
                std::make_shared<FocusNode>("TextField"),
                std::move(placeholder),
                false,
                std::move(on_changed),
                nullptr,
                std::move(key)) {}

class TextFieldState : public State<TextField> {
public:
    void init_state() override {
        effective_controller_ = widget().controller();
        if (!effective_controller_) {
            effective_controller_ = std::make_shared<TextEditingController>();
        }

        effective_focus_ = widget().focus_node();
        if (!effective_focus_) {
            effective_focus_ = std::make_shared<FocusNode>("TextField");
        }

        effective_controller_->add_listener([this](const TextEditingValue& /*val*/) {
            set_state([]() {});
        });

        effective_focus_->add_listener([this](bool /*focused*/) {
            set_state([]() {});
        });

        effective_focus_->set_on_key([this](const pal::KeyboardEvent& ev) -> bool {
            if (ev.type != pal::EventType::KeyDown) return false;

            // Enter / Return
            if (ev.virtual_key == 13 || ev.virtual_key == 10 || ev.key_code == 0xFF0D) {
                if (widget().on_submitted()) {
                    widget().on_submitted()(effective_controller_->text());
                }
                return true;
            }

            // Backspace
            if (ev.virtual_key == 8 || ev.virtual_key == 127 || ev.key_code == 0xFF08) {
                effective_controller_->delete_backward();
                if (widget().on_changed()) {
                    widget().on_changed()(effective_controller_->text());
                }
                return true;
            }

            // Delete
            if (ev.virtual_key == 0xFFFF || ev.key_code == 0xFFFF) {
                effective_controller_->delete_forward();
                if (widget().on_changed()) {
                    widget().on_changed()(effective_controller_->text());
                }
                return true;
            }

            // Arrow Left
            if (ev.virtual_key == 0xFF51 || ev.virtual_key == 37) {
                bool shift = (ev.modifiers & pal::ModifierKey::Shift);
                effective_controller_->move_cursor_left(shift);
                return true;
            }

            // Arrow Right
            if (ev.virtual_key == 0xFF53 || ev.virtual_key == 39) {
                bool shift = (ev.modifiers & pal::ModifierKey::Shift);
                effective_controller_->move_cursor_right(shift);
                return true;
            }

            // Home
            if (ev.virtual_key == 0xFF50 || ev.virtual_key == 36) {
                bool shift = (ev.modifiers & pal::ModifierKey::Shift);
                effective_controller_->move_cursor_to_start(shift);
                return true;
            }

            // End
            if (ev.virtual_key == 0xFF57 || ev.virtual_key == 35) {
                bool shift = (ev.modifiers & pal::ModifierKey::Shift);
                effective_controller_->move_cursor_to_end(shift);
                return true;
            }

            // Printable Unicode Character Typing
            if (ev.unicode_codepoint >= 32 && ev.unicode_codepoint != 127) {
                std::string utf8_char;
                uint32_t cp = ev.unicode_codepoint;
                if (cp < 0x80) {
                    utf8_char.push_back(static_cast<char>(cp));
                } else if (cp < 0x800) {
                    utf8_char.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                    utf8_char.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                } else if (cp < 0x10000) {
                    utf8_char.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                    utf8_char.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    utf8_char.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                } else {
                    utf8_char.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                    utf8_char.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                    utf8_char.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    utf8_char.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }

                effective_controller_->insert_text(utf8_char);
                if (widget().on_changed()) {
                    widget().on_changed()(effective_controller_->text());
                }
                return true;
            }

            return false;
        });

        effective_focus_->set_on_text([this](const pal::TextInputEvent& ev) {
            if (ev.type == pal::TextEventType::CompositionStart ||
                ev.type == pal::TextEventType::CompositionUpdate) {
                effective_controller_->set_composition(ev.utf8_text,
                                                        static_cast<int32_t>(ev.selection_start),
                                                        static_cast<int32_t>(ev.selection_end));
            } else if (ev.type == pal::TextEventType::CompositionCommit) {
                effective_controller_->commit_composition(ev.utf8_text);
                if (widget().on_changed()) {
                    widget().on_changed()(effective_controller_->text());
                }
            } else if (ev.type == pal::TextEventType::CompositionEnd) {
                effective_controller_->cancel_composition();
            }
        });
    }

    std::shared_ptr<Widget> build(BuildContext& /*context*/) override {
        const auto& w = widget();
        std::string raw_text = effective_controller_ ? effective_controller_->text() : "";
        bool is_empty = raw_text.empty();
        bool has_focus = effective_focus_ && effective_focus_->has_focus();

        std::string display_str;
        Color text_color;

        if (is_empty) {
            display_str = w.placeholder();
            text_color = Color::from_argb(150, 148, 163, 184); // Slate-400
        } else if (w.obscure_text()) {
            size_t graphemes = UnicodeGraphemeUtils::count_graphemes(raw_text);
            for (size_t i = 0; i < graphemes; ++i) {
                display_str += "\u2022";
            }
            text_color = Color::from_argb(255, 248, 250, 252);
        } else {
            display_str = raw_text;
            text_color = Color::from_argb(255, 248, 250, 252);
        }

        if (has_focus) {
            display_str += "|";
        }

        auto text_widget = std::make_shared<Text>(display_str, 14.0f, text_color);

        Color bg_color = has_focus ? Color::from_argb(255, 30, 48, 75)
                                   : Color::from_argb(255, 30, 41, 59);

        auto inner_container = std::make_shared<Container>(
            BoxConstraints::infinity,
            42.0f,
            bg_color,
            EdgeInsets::symmetric(10.0f, 12.0f),
            6.0f,
            text_widget
        );

        return std::make_shared<GestureDetector>(
            inner_container,
            [this]() {
                if (effective_focus_) {
                    effective_focus_->request_focus();
                }
            }
        );
    }

private:
    std::shared_ptr<TextEditingController> effective_controller_;
    std::shared_ptr<FocusNode> effective_focus_;
};

std::unique_ptr<StateBase> TextField::create_state() const {
    return std::make_unique<TextFieldState>();
}

} // namespace pingdart::ui

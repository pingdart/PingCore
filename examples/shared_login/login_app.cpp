#include "login_app.h"
#include <algorithm>
#include <iostream>

namespace pingdart::examples {

SharedLoginApp::SharedLoginApp() {
    username_ctrl_ = std::make_shared<ui::TextEditingController>("developer@pingdart.dev");
    password_ctrl_ = std::make_shared<ui::TextEditingController>("secretPassword123");

    user_focus_ = std::make_shared<ui::FocusNode>("username_field", 0);
    pass_focus_ = std::make_shared<ui::FocusNode>("password_field", 1);
    btn_focus_  = std::make_shared<ui::FocusNode>("submit_button", 2);

    focus_manager_.register_node(user_focus_.get());
    focus_manager_.register_node(pass_focus_.get());
    focus_manager_.register_node(btn_focus_.get());

    input_dispatcher_ = std::make_unique<ui::UIInputDispatcher>(widget_tree_, focus_manager_);

    rebuild_ui();
}

void SharedLoginApp::set_screen_geometry(float width, float height, const SafeAreaInsets& insets) {
    width_ = width;
    height_ = height;
    insets_ = insets;
    rebuild_ui();
}

void SharedLoginApp::rebuild_ui() {
    // Dynamic Responsive Card Width: min(available width - horizontal padding, 440)
    float available_w = width_ - insets_.left - insets_.right;
    float card_width = std::min(available_w - 32.0f, 440.0f);
    if (card_width < 280.0f) card_width = 280.0f;

    auto login_card = std::make_shared<ui::Container>(
        card_width,
        480.0f,
        ui::Color::from_argb(255, 30, 41, 59), // Slate 800
        ui::EdgeInsets::all(24.0f),
        16.0f,
        std::make_shared<ui::Column>(
            std::vector<std::shared_ptr<const ui::Widget>>{
                // Multilingual Title & Welcome
                std::make_shared<ui::Text>("PingDart Shared Login", 20.0f, ui::Color::from_argb(255, 248, 250, 252), "sans-serif", true),
                std::make_shared<ui::SizedBox>(0.0f, 6.0f),
                std::make_shared<ui::Text>("నమస్కారం! మొబైల్ కు స్వాగతం", 13.0f, ui::Color::from_argb(255, 148, 163, 184)),
                std::make_shared<ui::SizedBox>(0.0f, 20.0f),

                // Username TextField
                std::make_shared<ui::Text>("Username / Email", 12.0f, ui::Color::from_argb(255, 203, 213, 225)),
                std::make_shared<ui::SizedBox>(0.0f, 6.0f),
                std::make_shared<ui::TextField>(username_ctrl_, user_focus_, "Enter email"),
                std::make_shared<ui::SizedBox>(0.0f, 14.0f),

                // Password TextField (Masked)
                std::make_shared<ui::Text>("Password", 12.0f, ui::Color::from_argb(255, 203, 213, 225)),
                std::make_shared<ui::SizedBox>(0.0f, 6.0f),
                std::make_shared<ui::TextField>(password_ctrl_, pass_focus_, "Enter password", true),
                std::make_shared<ui::SizedBox>(0.0f, 20.0f),

                // Sign In Button
                std::make_shared<ui::Button>(
                    is_logged_in_ ? "లాగిన్ అయ్యారు! (Logged In)" : "లాగిన్ అవ్వండి (Sign In)",
                    [this]() {
                        is_logged_in_ = true;
                    },
                    btn_focus_
                )
            },
            ui::MainAxisAlignment::Start,
            ui::CrossAxisAlignment::Stretch
        )
    );

    // Padding wrapper accounting for Safe Area (notches, navigation bars, dynamic islands)
    auto safe_area_wrapper = std::make_shared<ui::Padding>(
        ui::EdgeInsets{insets_.top, insets_.bottom, insets_.left, insets_.right},
        std::make_shared<ui::Center>(login_card)
    );

    auto app_root = std::make_shared<ui::Container>(
        ui::BoxConstraints::infinity,
        ui::BoxConstraints::infinity,
        ui::Color::from_argb(255, 15, 23, 42), // Slate 900
        ui::EdgeInsets::zero(),
        0.0f,
        safe_area_wrapper
    );

    widget_tree_.set_root_widget(app_root);
    layout();
}

void SharedLoginApp::layout() {
    widget_tree_.flush_layout(ui::Size{width_, height_}, true);
}

core::Result<rhi::CompositorStats> SharedLoginApp::render(
    rhi::IRenderDevice* device,
    rhi::ISwapchain* swapchain,
    rhi::Compositor* compositor)
{
    if (!device || !swapchain || !compositor) {
        return core::Status(core::StatusCode::InvalidArgument, "Null graphics subsystem passed to render");
    }

    auto dl = widget_tree_.flush_paint();
    auto backbuffer_res = swapchain->acquire_next_backbuffer();
    if (!backbuffer_res.has_value()) {
        return backbuffer_res.status();
    }

    auto stats_res = compositor->render_frame(
        dl.get(),
        backbuffer_res.value(),
        rhi::ColorClearValue{0.06f, 0.09f, 0.16f, 1.0f}
    );
    if (!stats_res.has_value()) {
        return stats_res.status();
    }

    auto present_st = swapchain->present(rhi::PresentMode::Mailbox);
    if (!present_st.ok()) {
        return present_st;
    }

    return stats_res.value();
}

void SharedLoginApp::handle_pointer_down(double x, double y, uint32_t pointer_id) {
    pal::PointerEvent ev;
    ev.type = pal::EventType::PointerDown;
    ev.x = x;
    ev.y = y;
    ev.pointer_id = pointer_id;
    ev.button = pal::PointerButton::Left;
    input_dispatcher_->dispatch_pointer_event(ev);
}

void SharedLoginApp::handle_pointer_move(double x, double y, uint32_t pointer_id) {
    pal::PointerEvent ev;
    ev.type = pal::EventType::PointerMove;
    ev.x = x;
    ev.y = y;
    ev.pointer_id = pointer_id;
    ev.button = pal::PointerButton::Left;
    input_dispatcher_->dispatch_pointer_event(ev);
}

void SharedLoginApp::handle_pointer_up(double x, double y, uint32_t pointer_id) {
    pal::PointerEvent ev;
    ev.type = pal::EventType::PointerUp;
    ev.x = x;
    ev.y = y;
    ev.pointer_id = pointer_id;
    ev.button = pal::PointerButton::Left;
    input_dispatcher_->dispatch_pointer_event(ev);
}

void SharedLoginApp::handle_pointer_cancel(uint32_t pointer_id) {
    pal::PointerEvent ev;
    ev.type = pal::EventType::PointerLeave;
    ev.pointer_id = pointer_id;
    input_dispatcher_->dispatch_pointer_event(ev);
}

void SharedLoginApp::handle_key_down(uint32_t virtual_key) {
    pal::KeyboardEvent ev;
    ev.type = pal::EventType::KeyDown;
    ev.virtual_key = virtual_key;
    input_dispatcher_->dispatch_keyboard_event(ev);
}

void SharedLoginApp::handle_text_input(std::string_view text) {
    if (focus_manager_.primary_focus()) {
        std::string label = focus_manager_.primary_focus()->debug_label();
        if (label == "username_field") {
            username_ctrl_->set_text(username_ctrl_->text() + std::string(text));
        } else if (label == "password_field") {
            password_ctrl_->set_text(password_ctrl_->text() + std::string(text));
        }
    }
}

std::string SharedLoginApp::username() const {
    return username_ctrl_->text();
}

std::string SharedLoginApp::password() const {
    return password_ctrl_->text();
}

std::string SharedLoginApp::focused_element() const {
    return focus_manager_.primary_focus() ? focus_manager_.primary_focus()->debug_label() : "";
}

} // namespace pingdart::examples

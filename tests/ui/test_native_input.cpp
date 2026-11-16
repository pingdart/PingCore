#include <gtest/gtest.h>
#include <pingdart/ui/input_dispatcher.h>
#include <pingdart/ui/widget_tree.h>
#include <pingdart/ui/layout_primitives.h>
#include <pingdart/ui/leaf_widgets.h>
#include <pingdart/ui/focus_node.h>
#include <pingdart/ui/text_editing_controller.h>

using namespace pingdart;
using namespace pingdart::ui;

namespace {

pal::KeyboardEvent make_key_event(uint32_t vk, uint32_t cp = 0, pal::ModifierKey mod = pal::ModifierKey::None) {
    pal::KeyboardEvent ev;
    ev.type = pal::EventType::KeyDown;
    ev.virtual_key = vk;
    ev.unicode_codepoint = cp;
    ev.modifiers = mod;
    return ev;
}

} // namespace

TEST(NativeInputTest, MouseClickFocusesUsernameField) {
    WidgetTree tree;
    FocusManager focus_mgr;
    UIInputDispatcher dispatcher(tree, focus_mgr);

    auto username_controller = std::make_shared<TextEditingController>();
    auto username_focus = std::make_shared<FocusNode>("username_focus", 0);
    focus_mgr.register_node(username_focus.get());

    auto field = std::make_shared<TextField>(
        username_controller,
        username_focus,
        "Enter username"
    );

    auto root = std::make_shared<Container>(
        300.0f, 42.0f,
        Color::from_argb(255, 30, 41, 59),
        EdgeInsets::zero(),
        0.0f,
        field
    );

    tree.set_root_widget(root);
    tree.flush_layout(Size{800.0f, 600.0f}, true);

    EXPECT_FALSE(username_focus->has_focus());
    EXPECT_EQ(focus_mgr.primary_focus(), nullptr);

    // Click inside the text field at (50, 20)
    pal::PointerEvent click;
    click.type = pal::EventType::PointerDown;
    click.x = 50.0;
    click.y = 20.0;
    click.button = pal::PointerButton::Left;

    dispatcher.dispatch_pointer_event(click);

    EXPECT_TRUE(username_focus->has_focus());
    EXPECT_EQ(focus_mgr.primary_focus(), username_focus.get());
}

TEST(NativeInputTest, TypingUpdatesVisibleTextAndTriggersChanged) {
    WidgetTree tree;
    FocusManager focus_mgr;
    UIInputDispatcher dispatcher(tree, focus_mgr);

    auto controller = std::make_shared<TextEditingController>();
    auto focus = std::make_shared<FocusNode>("field_focus", 0);
    focus_mgr.register_node(focus.get());

    std::string changed_text;
    auto field = std::make_shared<TextField>(
        controller,
        focus,
        "Username",
        false,
        [&changed_text](const std::string& val) {
            changed_text = val;
        }
    );

    tree.set_root_widget(field);
    tree.flush_layout(Size{400.0f, 100.0f}, true);

    focus->request_focus();
    EXPECT_TRUE(focus->has_focus());

    // Type "ping"
    dispatcher.dispatch_keyboard_event(make_key_event('p', 'p'));
    dispatcher.dispatch_keyboard_event(make_key_event('i', 'i'));
    dispatcher.dispatch_keyboard_event(make_key_event('n', 'n'));
    dispatcher.dispatch_keyboard_event(make_key_event('g', 'g'));

    EXPECT_EQ(controller->text(), "ping");
    EXPECT_EQ(changed_text, "ping");
}

TEST(NativeInputTest, TabMovesFocusForwardAndShiftTabMovesBackward) {
    WidgetTree tree;
    FocusManager focus_mgr;
    UIInputDispatcher dispatcher(tree, focus_mgr);

    auto focus_user = std::make_shared<FocusNode>("user", 0);
    auto focus_pass = std::make_shared<FocusNode>("pass", 1);
    auto focus_btn  = std::make_shared<FocusNode>("btn", 2);

    focus_mgr.register_node(focus_user.get());
    focus_mgr.register_node(focus_pass.get());
    focus_mgr.register_node(focus_btn.get());

    focus_user->request_focus();
    ASSERT_EQ(focus_mgr.primary_focus(), focus_user.get());

    // Send Tab key -> should advance to focus_pass
    dispatcher.dispatch_keyboard_event(make_key_event(9));
    EXPECT_EQ(focus_mgr.primary_focus(), focus_pass.get());
    EXPECT_TRUE(focus_pass->has_focus());
    EXPECT_FALSE(focus_user->has_focus());

    // Send Tab key again -> should advance to focus_btn
    dispatcher.dispatch_keyboard_event(make_key_event(9));
    EXPECT_EQ(focus_mgr.primary_focus(), focus_btn.get());
    EXPECT_TRUE(focus_btn->has_focus());

    // Send Shift+Tab key -> should move backward to focus_pass
    dispatcher.dispatch_keyboard_event(make_key_event(9, 0, pal::ModifierKey::Shift));
    EXPECT_EQ(focus_mgr.primary_focus(), focus_pass.get());

    // Send Shift+Tab again -> moves back to focus_user
    dispatcher.dispatch_keyboard_event(make_key_event(9, 0, pal::ModifierKey::Shift));
    EXPECT_EQ(focus_mgr.primary_focus(), focus_user.get());
}

TEST(NativeInputTest, EnterActivatesFocusedButton) {
    WidgetTree tree;
    FocusManager focus_mgr;
    UIInputDispatcher dispatcher(tree, focus_mgr);

    auto button_focus = std::make_shared<FocusNode>("submit_btn", 0);
    focus_mgr.register_node(button_focus.get());

    bool button_clicked = false;
    auto btn = std::make_shared<Button>(
        "Submit",
        [&button_clicked]() {
            button_clicked = true;
        },
        button_focus
    );

    tree.set_root_widget(btn);
    tree.flush_layout(Size{200.0f, 50.0f}, true);

    button_focus->request_focus();
    ASSERT_TRUE(button_focus->has_focus());

    // Dispatch Enter key (virtual_key = 13)
    dispatcher.dispatch_keyboard_event(make_key_event(13));
    EXPECT_TRUE(button_clicked);
}

TEST(NativeInputTest, BackspaceDeletesCompleteGraphemeClustersInTelugu) {
    TextEditingController controller;

    // Telugu text "నమస్కారం"
    // Grapheme clusters:
    // 1. "న" (Na)
    // 2. "మ" (Ma)
    // 3. "స్కా" (Sa + Virama + Ka + Matra AA)
    // 4. "రం" (Ra + Anusvara)
    controller.set_text("నమస్కారం");
    EXPECT_EQ(UnicodeGraphemeUtils::count_graphemes(controller.text()), 4u);

    // Cursor is at the end (after "రం")
    controller.delete_backward();
    EXPECT_EQ(controller.text(), "నమస్కా");
    EXPECT_EQ(UnicodeGraphemeUtils::count_graphemes(controller.text()), 3u);

    // Next backspace deletes the full conjunct "స్కా" in one user action!
    controller.delete_backward();
    EXPECT_EQ(controller.text(), "నమ");
    EXPECT_EQ(UnicodeGraphemeUtils::count_graphemes(controller.text()), 2u);

    controller.delete_backward();
    EXPECT_EQ(controller.text(), "న");

    controller.delete_backward();
    EXPECT_EQ(controller.text(), "");
}

TEST(NativeInputTest, TeluguImeCompositionAndCommit) {
    WidgetTree tree;
    FocusManager focus_mgr;
    UIInputDispatcher dispatcher(tree, focus_mgr);

    auto controller = std::make_shared<TextEditingController>();
    auto focus = std::make_shared<FocusNode>("ime_field", 0);
    focus_mgr.register_node(focus.get());

    std::string submitted;
    auto field = std::make_shared<TextField>(
        controller,
        focus,
        "Enter Telugu",
        false,
        nullptr,
        [&submitted](const std::string& val) {
            submitted = val;
        }
    );

    tree.set_root_widget(field);
    tree.flush_layout(Size{400.0f, 60.0f}, true);
    focus->request_focus();

    // 1. Start IME composition
    pal::TextInputEvent start_ev;
    start_ev.type = pal::TextEventType::CompositionStart;
    start_ev.utf8_text = "నమ";
    dispatcher.dispatch_text_event(start_ev);
    EXPECT_EQ(controller->text(), "నమ");
    EXPECT_TRUE(controller->value().composing_range.is_valid());

    // 2. Update IME composition
    pal::TextInputEvent update_ev;
    update_ev.type = pal::TextEventType::CompositionUpdate;
    update_ev.utf8_text = "నమస్కారం";
    dispatcher.dispatch_text_event(update_ev);
    EXPECT_EQ(controller->text(), "నమస్కారం");

    // 3. Commit IME composition
    pal::TextInputEvent commit_ev;
    commit_ev.type = pal::TextEventType::CompositionCommit;
    commit_ev.utf8_text = "నమస్కారం";
    dispatcher.dispatch_text_event(commit_ev);

    EXPECT_EQ(controller->text(), "నమస్కారం");
    EXPECT_FALSE(controller->value().composing_range.is_valid());

    // 4. Press Enter to submit
    dispatcher.dispatch_keyboard_event(make_key_event(13));
    EXPECT_EQ(submitted, "నమస్కారం");
}

TEST(NativeInputTest, ExtendedGraphemeClusterEmojiAndFlags) {
    TextEditingController controller;

    // 1. Regional Indicator Flag: India flag (U+1F1EE U+1F1F3) = 1 grapheme cluster (8 bytes)
    controller.set_text("\xF0\x9F\x87\xAE\xF0\x9F\x87\xB3");
    EXPECT_EQ(UnicodeGraphemeUtils::count_graphemes(controller.text()), 1u);
    controller.delete_backward();
    EXPECT_EQ(controller.text(), "");

    // 2. Emoji ZWJ sequence: Family (Man + ZWJ + Woman + ZWJ + Girl + ZWJ + Boy) = 1 grapheme
    std::string family_emoji = "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6";
    controller.set_text(family_emoji);
    EXPECT_EQ(UnicodeGraphemeUtils::count_graphemes(controller.text()), 1u);
    controller.delete_backward();
    EXPECT_EQ(controller.text(), "");

    // 3. CRLF sequence \r\n = 1 grapheme
    controller.set_text("Line1\r\nLine2");
    EXPECT_EQ(UnicodeGraphemeUtils::count_graphemes(controller.text()), 11u);
}

TEST(NativeInputTest, PasswordInputMasksCharactersWithBullets) {
    auto controller = std::make_shared<TextEditingController>("MySecret123");
    auto focus = std::make_shared<FocusNode>("password", 0);

    auto field = std::make_shared<TextField>(
        controller,
        focus,
        "Password",
        true // obscure_text = true
    );

    WidgetTree tree;
    tree.set_root_widget(field);
    tree.flush_layout(Size{300.0f, 50.0f}, true);

    // Build outputs text widget with bullet symbols
    EXPECT_EQ(controller->text(), "MySecret123");
    EXPECT_EQ(UnicodeGraphemeUtils::count_graphemes(controller->text()), 11u);
}

TEST(NativeInputTest, SelectionAndCaretNavigation) {
    TextEditingController controller("Hello World");
    controller.set_selection(TextSelection{0, 5}); // Select "Hello"

    EXPECT_FALSE(controller.selection().is_collapsed());
    EXPECT_EQ(controller.selection().start(), 0);
    EXPECT_EQ(controller.selection().end(), 5);

    // Replace selected range
    controller.insert_text("PingDart");
    EXPECT_EQ(controller.text(), "PingDart World");
    EXPECT_EQ(controller.selection().base_offset, 8);

    // Move to end
    controller.move_cursor_to_end();
    EXPECT_EQ(controller.selection().base_offset, static_cast<int32_t>(controller.text().size()));

    // Select all
    controller.select_all();
    EXPECT_EQ(controller.selection().start(), 0);
    EXPECT_EQ(controller.selection().end(), static_cast<int32_t>(controller.text().size()));
}

TEST(NativeInputTest, WindowResizeLifecycleRecalculatesWidgetLayout) {
    WidgetTree tree;
    FocusManager focus_mgr;
    UIInputDispatcher dispatcher(tree, focus_mgr);

    auto root = std::make_shared<Container>(
        BoxConstraints::infinity,
        BoxConstraints::infinity,
        Color::from_argb(255, 15, 23, 42)
    );

    tree.set_root_widget(root);
    tree.flush_layout(Size{800.0f, 600.0f}, true);

    ASSERT_NE(tree.root_render_box(), nullptr);
    EXPECT_FLOAT_EQ(tree.root_render_box()->size().width, 800.0f);
    EXPECT_FLOAT_EQ(tree.root_render_box()->size().height, 600.0f);

    // OS Window resized to 1920x1080
    pal::WindowLifecycleEvent resize_ev;
    resize_ev.type = pal::EventType::WindowResized;
    resize_ev.width = 1920;
    resize_ev.height = 1080;

    dispatcher.dispatch_lifecycle_event(resize_ev);

    EXPECT_FLOAT_EQ(tree.root_render_box()->size().width, 1920.0f);
    EXPECT_FLOAT_EQ(tree.root_render_box()->size().height, 1080.0f);
}

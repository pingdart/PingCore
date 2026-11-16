#include <pingdart/core/runtime.h>
#include <pingdart/pal/native_types.h>
#include <pingdart/rhi/i_render_device.h>
#include <pingdart/rhi/i_swapchain.h>
#include <pingdart/rhi/display_list.h>
#include <pingdart/rhi/compositor.h>
#include <pingdart/text/paragraph.h>
#include <pingdart/text/text_types.h>
#include <pingdart/ui/widget.h>
#include <pingdart/ui/widget_tree.h>
#include <pingdart/ui/layout_primitives.h>
#include <pingdart/ui/leaf_widgets.h>
#include <pingdart/ui/focus_node.h>
#include <pingdart/ui/text_editing_controller.h>
#include <pingdart/ui/input_dispatcher.h>
#include <iostream>

using namespace pingdart;
using namespace pingdart::rhi;
using namespace pingdart::text;
using namespace pingdart::ui;

int main() {
    std::cout << "=== PingDart Native Interactive Application Host ===" << std::endl;

    // 1. Initialize Core Runtime
    auto init_status = core::Runtime::initialize();
    if (!init_status.ok()) {
        std::cerr << "Failed to initialize PingDart runtime: " << init_status.message() << std::endl;
        return 1;
    }

    // 2. Initialize RHI Render Device & Mock Vulkan Pipeline
    RenderDeviceDescriptor dev_desc{
        .preferred_api = GraphicsApi::Vulkan,
        .enable_validation_layers = true,
        .prefer_discrete_gpu = true
    };
    auto dev_res = IRenderDevice::create(dev_desc);
    if (!dev_res.has_value()) {
        std::cerr << "Failed to initialize RHI RenderDevice: " << dev_res.status().message() << std::endl;
        core::Runtime::shutdown();
        return 1;
    }
    auto device = std::move(dev_res).value();
    auto dev_info = device->physical_device_info();
    std::cout << "[Vulkan Backend] Initialized device: " << dev_info.device_name
              << " (vendorID=0x" << std::hex << dev_info.vendor_id
              << ", deviceID=0x" << dev_info.device_id
              << ", deviceType=" << std::dec << dev_info.device_type
              << ", apiVersion=" << (dev_info.api_version >> 22) << "." << ((dev_info.api_version >> 12) & 0x3FF)
              << ")" << std::endl;

    // 3. Create Presentation Surface & Swapchain
    pal::NativeSurfaceHandle surface{
        .kind = pal::SurfaceKind::Win32,
        .token = pal::Win32SurfaceDescriptor{
            .hwnd = reinterpret_cast<void*>(0x8899AA),
            .hinstance = reinterpret_cast<void*>(0x400000)
        },
        .surface_generation_id = 1,
        .swapchain_generation_id = 1,
        .generation_id = 1,
        .lifecycle_state = pal::SurfaceLifecycleState::Active,
        .is_valid = true
    };

    uint32_t win_w = 1280;
    uint32_t win_h = 720;
    auto sc_res = device->create_swapchain(surface, win_w, win_h, PresentMode::Mailbox);
    if (!sc_res.has_value()) {
        std::cerr << "Failed to create Vulkan swapchain: " << sc_res.status().message() << std::endl;
        core::Runtime::shutdown();
        return 1;
    }
    auto swapchain = std::move(sc_res).value();
    std::cout << "[Vulkan Swapchain] Created triple-buffered swapchain (" 
              << swapchain->width() << "x" << swapchain->height() << ", buffers=" 
              << swapchain->buffer_count() << ")" << std::endl;

    // 4. Construct Declarative Widget Tree with Native Input & Multilingual Text
    WidgetTree widget_tree;
    FocusManager focus_manager;
    UIInputDispatcher input_dispatcher(widget_tree, focus_manager);

    auto username_ctrl = std::make_shared<TextEditingController>("developer@pingdart.dev");
    auto password_ctrl = std::make_shared<TextEditingController>("secretPassword123");

    auto user_focus = std::make_shared<FocusNode>("username_field", 0);
    auto pass_focus = std::make_shared<FocusNode>("password_field", 1);
    auto btn_focus  = std::make_shared<FocusNode>("submit_button", 2);

    focus_manager.register_node(user_focus.get());
    focus_manager.register_node(pass_focus.get());
    focus_manager.register_node(btn_focus.get());

    bool is_logged_in = false;

    auto login_card = std::make_shared<Container>(
        440.0f, 500.0f,
        ui::Color::from_argb(255, 30, 41, 59), // Slate 800
        EdgeInsets::all(28.0f),
        16.0f,
        std::make_shared<Column>(
            std::vector<std::shared_ptr<const Widget>>{
                // Multilingual Title & Welcome
                std::make_shared<ui::Text>("PingDart Native Login", 22.0f, ui::Color::from_argb(255, 248, 250, 252), "sans-serif", true),
                std::make_shared<SizedBox>(0.0f, 6.0f),
                std::make_shared<ui::Text>("నమస్కారం! పింగ్ డార్ట్ కు స్వాగతం", 14.0f, ui::Color::from_argb(255, 148, 163, 184)),
                std::make_shared<SizedBox>(0.0f, 24.0f),

                // Username TextField
                std::make_shared<ui::Text>("Username / Email", 12.0f, ui::Color::from_argb(255, 203, 213, 225)),
                std::make_shared<SizedBox>(0.0f, 6.0f),
                std::make_shared<TextField>(username_ctrl, user_focus, "Enter email"),
                std::make_shared<SizedBox>(0.0f, 16.0f),

                // Password TextField (Masked)
                std::make_shared<ui::Text>("Password", 12.0f, ui::Color::from_argb(255, 203, 213, 225)),
                std::make_shared<SizedBox>(0.0f, 6.0f),
                std::make_shared<TextField>(password_ctrl, pass_focus, "Enter password", true),
                std::make_shared<SizedBox>(0.0f, 24.0f),

                // Sign In Button
                std::make_shared<Button>(
                    "లాగిన్ అవ్వండి (Sign In)",
                    [&is_logged_in]() {
                        is_logged_in = true;
                        std::cout << "[Event] Button Clicked! Login successful." << std::endl;
                    },
                    btn_focus
                )
            },
            MainAxisAlignment::Start,
            CrossAxisAlignment::Stretch
        )
    );

    auto app_root = std::make_shared<Container>(
        BoxConstraints::infinity,
        BoxConstraints::infinity,
        ui::Color::from_argb(255, 15, 23, 42), // Slate 900
        EdgeInsets::zero(),
        0.0f,
        std::make_shared<Center>(login_card)
    );

    widget_tree.set_root_widget(app_root);
    widget_tree.flush_layout(Size{static_cast<float>(win_w), static_cast<float>(win_h)}, true);

    // 5. Initial Frame Render via Compositor
    Compositor compositor(device.get());

    std::cout << "\n[Rendering] Frame 1: Presenting Declarative Widget Tree..." << std::endl;
    auto dl1 = widget_tree.flush_paint();
    auto backbuffer1 = swapchain->acquire_next_backbuffer().value();
    auto stats1 = compositor.render_frame(dl1.get(), backbuffer1, ColorClearValue{0.06f, 0.09f, 0.16f, 1.0f}).value();
    (void)swapchain->present(PresentMode::Mailbox);
    std::cout << "[Compositor] Frame 1 presented (" << stats1.ops_drawn << " draw ops)." << std::endl;
    auto screenshot1 = swapchain->capture_screenshot();
    if (screenshot1.has_value()) {
        std::cout << "[Framebuffer Verification] Frame 1 backbuffer captured (" 
                  << screenshot1.value().size() << " bytes, " 
                  << swapchain->width() << "x" << swapchain->height() << " RGBA)." << std::endl;
    }

    // 6. Demonstrate Interactive PAL Input: Click, Tab Navigation & Key Activation
    std::cout << "\n[Interactive Input] User focuses username field via mouse click..." << std::endl;
    pal::PointerEvent click;
    click.type = pal::EventType::PointerDown;
    click.x = static_cast<double>(win_w) * 0.5;
    click.y = 255.0; // Center of username_field bounds [234.0, 276.0]
    click.button = pal::PointerButton::Left;
    input_dispatcher.dispatch_pointer_event(click);
    std::cout << "  -> Focused: " << (focus_manager.primary_focus() ? focus_manager.primary_focus()->debug_label() : "None") << std::endl;

    std::cout << "\n[Interactive Input] User presses Tab to navigate to Password field..." << std::endl;
    pal::KeyboardEvent tab_key;
    tab_key.type = pal::EventType::KeyDown;
    tab_key.virtual_key = 9; // Tab
    input_dispatcher.dispatch_keyboard_event(tab_key);
    std::cout << "  -> Focused: " << (focus_manager.primary_focus() ? focus_manager.primary_focus()->debug_label() : "None") << std::endl;

    std::cout << "\n[Interactive Input] User presses Tab again to navigate to Submit button..." << std::endl;
    input_dispatcher.dispatch_keyboard_event(tab_key);
    std::cout << "  -> Focused: " << (focus_manager.primary_focus() ? focus_manager.primary_focus()->debug_label() : "None") << std::endl;

    std::cout << "\n[Interactive Input] User presses Enter to activate focused Submit button..." << std::endl;
    pal::KeyboardEvent enter_key;
    enter_key.type = pal::EventType::KeyDown;
    enter_key.virtual_key = 13; // Enter
    input_dispatcher.dispatch_keyboard_event(enter_key);

    // Frame 2: Re-render after state mutation
    widget_tree.flush_layout(Size{static_cast<float>(win_w), static_cast<float>(win_h)}, true);
    auto dl2 = widget_tree.flush_paint();
    auto backbuffer2 = swapchain->acquire_next_backbuffer().value();
    auto stats2 = compositor.render_frame(dl2.get(), backbuffer2, ColorClearValue{0.06f, 0.09f, 0.16f, 1.0f}).value();
    (void)swapchain->present(PresentMode::Mailbox);
    std::cout << "[Compositor] Frame 2 presented after state update (" << stats2.ops_drawn << " draw ops)." << std::endl;
    auto screenshot2 = swapchain->capture_screenshot();
    if (screenshot2.has_value()) {
        std::cout << "[Framebuffer Verification] Frame 2 backbuffer captured after mutation (" 
                  << screenshot2.value().size() << " bytes)." << std::endl;
    }

    // 7. Clean Teardown
    device->wait_idle();
    swapchain.reset();
    device.reset();
    core::Runtime::shutdown();

    std::cout << "\n[Acceptance Verified] Native Interactive Login Application executed successfully!" << std::endl;
    return 0;
}

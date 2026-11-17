#pragma once

#include <pingdart/core/runtime.h>
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
#include <memory>
#include <string>

namespace pingdart::examples {

struct SafeAreaInsets {
    float top{0.0f};
    float bottom{0.0f};
    float left{0.0f};
    float right{0.0f};
};

class SharedLoginApp {
public:
    SharedLoginApp();
    ~SharedLoginApp() = default;

    /// Configures screen dimensions and safe area insets (notches, navigation bars, dynamic islands)
    void set_screen_geometry(float width, float height, const SafeAreaInsets& insets = {});

    /// Triggers layout pass
    void layout();

    /// Renders the current frame to the swapchain via the compositor
    core::Result<rhi::CompositorStats> render(
        rhi::IRenderDevice* device,
        rhi::ISwapchain* swapchain,
        rhi::Compositor* compositor
    );

    /// Input handling
    void handle_pointer_down(double x, double y, uint32_t pointer_id = 0);
    void handle_pointer_move(double x, double y, uint32_t pointer_id = 0);
    void handle_pointer_up(double x, double y, uint32_t pointer_id = 0);
    void handle_pointer_cancel(uint32_t pointer_id = 0);
    void handle_key_down(uint32_t virtual_key);
    void handle_text_input(std::string_view text);

    /// Application state
    [[nodiscard]] bool is_logged_in() const noexcept { return is_logged_in_; }
    [[nodiscard]] std::string username() const;
    [[nodiscard]] std::string password() const;
    [[nodiscard]] std::string focused_element() const;
    [[nodiscard]] float width() const noexcept { return width_; }
    [[nodiscard]] float height() const noexcept { return height_; }
    [[nodiscard]] const SafeAreaInsets& safe_area_insets() const noexcept { return insets_; }

    /// Rebuilds widget tree
    void rebuild_ui();

private:
    float width_{393.0f};
    float height_{852.0f};
    SafeAreaInsets insets_{};
    bool is_logged_in_{false};

    ui::WidgetTree widget_tree_;
    ui::FocusManager focus_manager_;
    std::unique_ptr<ui::UIInputDispatcher> input_dispatcher_;

    std::shared_ptr<ui::TextEditingController> username_ctrl_;
    std::shared_ptr<ui::TextEditingController> password_ctrl_;

    std::shared_ptr<ui::FocusNode> user_focus_;
    std::shared_ptr<ui::FocusNode> pass_focus_;
    std::shared_ptr<ui::FocusNode> btn_focus_;
};

} // namespace pingdart::examples

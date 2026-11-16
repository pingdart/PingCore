#pragma once

#include "display_list.h"
#include "i_render_device.h"
#include "i_command_buffer.h"
#include "i_texture.h"
#include <pingdart/core/result.h>
#include <cstdint>

namespace pingdart::rhi {

struct CompositorStats {
    uint32_t total_ops_evaluated{0};
    uint32_t ops_drawn{0};
    uint32_t ops_culled_by_damage{0};
    Rect rendered_damage_rect{};
};

class Compositor {
public:
    explicit Compositor(IRenderDevice* device);
    ~Compositor() = default;

    Compositor(const Compositor&) = delete;
    Compositor& operator=(const Compositor&) = delete;

    // Renders the display list to the specified render target
    // Partial redraw optimization is enabled ONLY when persistent_surface is true
    // (e.g. persistent off-screen texture or buffer-age validated backbuffer)
    [[nodiscard]] core::Result<CompositorStats> render_frame(
        const DisplayList* display_list,
        ITexture* render_target,
        const ColorClearValue& clear_color,
        const DamageRegion* damage = nullptr,
        bool persistent_surface = false
    );

private:
    IRenderDevice* device_{nullptr};
};

} // namespace pingdart::rhi

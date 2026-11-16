#pragma once

#include "rhi_types.h"
#include <pingdart/core/result.h>
#include <cstdint>

namespace pingdart::rhi {

class ITexture;
class IPipelineState;

enum class CommandBufferState : uint8_t {
    Initial,
    Recording,
    Executable,
    Submitted
};

class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;

    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void reset() = 0;

    virtual void begin_render_pass(
        ITexture* render_target,
        const ColorClearValue& clear_color) = 0;
    virtual void end_render_pass() = 0;

    virtual void set_viewport(const Viewport& viewport) = 0;
    virtual void set_scissor(const ScissorRect& scissor) = 0;

    virtual void bind_pipeline(IPipelineState* pipeline) = 0;

    virtual void draw(uint32_t vertex_count, uint32_t first_vertex = 0) = 0;
    virtual void draw_indexed(
        uint32_t index_count,
        uint32_t first_index = 0,
        int32_t vertex_offset = 0) = 0;

    [[nodiscard]] virtual CommandBufferState state() const noexcept = 0;
};

} // namespace pingdart::rhi

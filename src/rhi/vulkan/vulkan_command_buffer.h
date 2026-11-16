#pragma once

#include <pingdart/rhi/i_command_buffer.h>
#include <pingdart/rhi/i_texture.h>
#include <pingdart/rhi/i_pipeline_state.h>
#include "vulkan_types.h"
#include "vulkan_dispatcher.h"

namespace pingdart::rhi::vk {

class VulkanCommandBuffer final : public ICommandBuffer {
public:
    VulkanCommandBuffer(IVulkanDispatcher* dispatcher, VkDevice device, VkCommandBuffer cmd_buf);
    ~VulkanCommandBuffer() override = default;

    void begin() override;
    void end() override;
    void reset() override;

    void begin_render_pass(
        ITexture* render_target,
        const ColorClearValue& clear_color) override;
    void end_render_pass() override;

    void set_viewport(const Viewport& viewport) override;
    void set_scissor(const ScissorRect& scissor) override;

    void bind_pipeline(IPipelineState* pipeline) override;

    void draw(uint32_t vertex_count, uint32_t first_vertex = 0) override;
    void draw_indexed(
        uint32_t index_count,
        uint32_t first_index = 0,
        int32_t vertex_offset = 0) override;

    [[nodiscard]] CommandBufferState state() const noexcept override { return state_; }
    [[nodiscard]] VkCommandBuffer handle() const noexcept { return cmd_buf_; }

    void mark_submitted() noexcept { state_ = CommandBufferState::Submitted; }

private:
    IVulkanDispatcher* dispatcher_{nullptr};
    [[maybe_unused]] VkDevice device_{VK_NULL_HANDLE};
    VkCommandBuffer cmd_buf_{VK_NULL_HANDLE};
    CommandBufferState state_{CommandBufferState::Initial};
};

} // namespace pingdart::rhi::vk

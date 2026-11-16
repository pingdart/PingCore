#include "vulkan_command_buffer.h"
#include "vulkan_texture.h"

namespace pingdart::rhi::vk {

VulkanCommandBuffer::VulkanCommandBuffer(
    IVulkanDispatcher* dispatcher,
    VkDevice device,
    VkCommandBuffer cmd_buf)
    : dispatcher_(dispatcher)
    , device_(device)
    , cmd_buf_(cmd_buf)
{
}

void VulkanCommandBuffer::begin() {
    if (state_ == CommandBufferState::Recording) return;
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    dispatcher_->begin_command_buffer(cmd_buf_, &begin_info);
    state_ = CommandBufferState::Recording;
}

void VulkanCommandBuffer::end() {
    if (state_ != CommandBufferState::Recording) return;
    dispatcher_->end_command_buffer(cmd_buf_);
    state_ = CommandBufferState::Executable;
}

void VulkanCommandBuffer::reset() {
    state_ = CommandBufferState::Initial;
}

void VulkanCommandBuffer::begin_render_pass(
    ITexture* render_target,
    const ColorClearValue& clear_color)
{
    if (state_ != CommandBufferState::Recording) return;

    VkRenderPassBeginInfo rp_info{};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_info.renderArea.offset = {0, 0};
    rp_info.renderArea.extent = {
        render_target ? render_target->width() : 0,
        render_target ? render_target->height() : 0
    };

    VkClearValue clear{};
    clear.color.float32[0] = clear_color.r;
    clear.color.float32[1] = clear_color.g;
    clear.color.float32[2] = clear_color.b;
    clear.color.float32[3] = clear_color.a;

    rp_info.clearValueCount = 1;
    rp_info.pClearValues = &clear;

    dispatcher_->cmd_begin_render_pass(cmd_buf_, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::end_render_pass() {
    if (state_ != CommandBufferState::Recording) return;
    dispatcher_->cmd_end_render_pass(cmd_buf_);
}

void VulkanCommandBuffer::set_viewport(const Viewport& viewport) {
    if (state_ != CommandBufferState::Recording) return;
    VkViewport vp{};
    vp.x = viewport.x;
    vp.y = viewport.y;
    vp.width = viewport.width;
    vp.height = viewport.height;
    vp.minDepth = viewport.min_depth;
    vp.maxDepth = viewport.max_depth;
    dispatcher_->cmd_set_viewport(cmd_buf_, 0, 1, &vp);
}

void VulkanCommandBuffer::set_scissor(const ScissorRect& scissor) {
    if (state_ != CommandBufferState::Recording) return;
    VkRect2D sc{};
    sc.offset = {scissor.x, scissor.y};
    sc.extent = {scissor.width, scissor.height};
    dispatcher_->cmd_set_scissor(cmd_buf_, 0, 1, &sc);
}

void VulkanCommandBuffer::bind_pipeline(IPipelineState*) {
    // In full pipeline state, this binds VkPipeline via vkCmdBindPipeline
}

void VulkanCommandBuffer::draw(uint32_t vertex_count, uint32_t first_vertex) {
    if (state_ != CommandBufferState::Recording) return;
    dispatcher_->cmd_draw(cmd_buf_, vertex_count, 1, first_vertex, 0);
}

void VulkanCommandBuffer::draw_indexed(uint32_t index_count, uint32_t first_index, int32_t) {
    if (state_ != CommandBufferState::Recording) return;
    dispatcher_->cmd_draw(cmd_buf_, index_count, 1, first_index, 0);
}

} // namespace pingdart::rhi::vk

#pragma once

#include <pingdart/rhi/i_swapchain.h>
#include "vulkan_types.h"
#include "vulkan_dispatcher.h"
#include "vulkan_surface.h"
#include "vulkan_texture.h"
#include <vector>
#include <memory>

namespace pingdart::rhi::vk {

class VulkanSwapchain final : public ISwapchain {
public:
    static core::Result<std::unique_ptr<VulkanSwapchain>> create(
        IVulkanDispatcher* dispatcher,
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkQueue present_queue,
        uint32_t queue_family_index,
        std::unique_ptr<VulkanSurface> surface,
        uint32_t width,
        uint32_t height,
        PresentMode preferred_mode
    );

    ~VulkanSwapchain() override;

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    [[nodiscard]] core::Status resize(uint32_t width, uint32_t height) override;
    [[nodiscard]] core::Result<ITexture*> acquire_next_backbuffer() override;
    [[nodiscard]] core::Status present(PresentMode mode = PresentMode::Fifo) override;
    [[nodiscard]] core::Result<std::vector<uint8_t>> capture_screenshot() const override;

    [[nodiscard]] uint32_t width() const noexcept override { return width_; }
    [[nodiscard]] uint32_t height() const noexcept override { return height_; }
    [[nodiscard]] PixelFormat format() const noexcept override { return PixelFormat::BGRA8_UNORM; }
    [[nodiscard]] uint32_t buffer_count() const noexcept override {
        return static_cast<uint32_t>(backbuffers_.size());
    }
    [[nodiscard]] bool is_valid() const noexcept override {
        return is_valid_ && width_ > 0 && height_ > 0;
    }
    [[nodiscard]] uint64_t swapchain_generation_id() const noexcept override {
        return swapchain_generation_id_;
    }

    [[nodiscard]] VkSwapchainKHR handle() const noexcept { return swapchain_; }
    [[nodiscard]] VulkanSurface* surface() const noexcept { return surface_.get(); }
    [[nodiscard]] bool is_out_of_date() const noexcept { return out_of_date_; }

private:
    VulkanSwapchain(
        IVulkanDispatcher* dispatcher,
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkQueue present_queue,
        uint32_t queue_family_index,
        std::unique_ptr<VulkanSurface> surface,
        uint32_t width,
        uint32_t height,
        PresentMode preferred_mode
    );

    core::Status init_or_recreate(VkSwapchainKHR old_swapchain);
    void cleanup_swapchain_resources();

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    IVulkanDispatcher* dispatcher_{nullptr};
    VkDevice device_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    VkQueue present_queue_{VK_NULL_HANDLE};
    uint32_t queue_family_index_{0};

    std::unique_ptr<VulkanSurface> surface_;
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    VkFormat image_format_{VK_FORMAT_B8G8R8A8_UNORM};
    VkColorSpaceKHR color_space_{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

    uint32_t width_{0};
    uint32_t height_{0};
    PresentMode preferred_mode_{PresentMode::Fifo};
    uint64_t swapchain_generation_id_{1};
    bool is_valid_{false};
    bool out_of_date_{false};

    uint32_t current_frame_{0};
    uint32_t current_image_index_{0};

    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    std::vector<std::unique_ptr<VulkanTexture>> backbuffers_;

    std::vector<VkSemaphore> image_acquired_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
};

} // namespace pingdart::rhi::vk

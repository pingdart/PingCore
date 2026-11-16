#pragma once

#include "vulkan_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace pingdart::rhi::vk {

class IVulkanDispatcher {
public:
    virtual ~IVulkanDispatcher() = default;

    virtual VkResult create_instance(const VkInstanceCreateInfo* pCreateInfo, const void* pAllocator, VkInstance* pInstance) = 0;
    virtual void destroy_instance(VkInstance instance, const void* pAllocator) = 0;

    virtual VkResult enumerate_physical_devices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) = 0;
    virtual void get_physical_device_properties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) = 0;
    virtual void get_physical_device_queue_family_properties(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties) = 0;

    virtual VkResult get_physical_device_surface_support(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported) = 0;
    virtual VkResult get_physical_device_surface_capabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) = 0;
    virtual VkResult get_physical_device_surface_formats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats) = 0;
    virtual VkResult get_physical_device_surface_present_modes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes) = 0;

    virtual VkResult create_device(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const void* pAllocator, VkDevice* pDevice) = 0;
    virtual void destroy_device(VkDevice device, const void* pAllocator) = 0;

    virtual void get_device_queue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue) = 0;
    virtual VkResult device_wait_idle(VkDevice device) = 0;
    virtual VkResult queue_wait_idle(VkQueue queue) = 0;

    virtual VkResult create_win32_surface(VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface) = 0;
    virtual VkResult create_wayland_surface(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface) = 0;
    virtual VkResult create_xcb_surface(VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface) = 0;
    virtual VkResult create_xlib_surface(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface) = 0;
    virtual VkResult create_metal_surface(VkInstance instance, const VkMetalSurfaceCreateInfoEXT* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface) = 0;
    virtual void destroy_surface(VkInstance instance, VkSurfaceKHR surface, const void* pAllocator) = 0;

    virtual VkResult create_swapchain(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSwapchainKHR* pSwapchain) = 0;
    virtual void destroy_swapchain(VkDevice device, VkSwapchainKHR swapchain, const void* pAllocator) = 0;
    virtual VkResult get_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages) = 0;
    virtual VkResult acquire_next_image(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) = 0;
    virtual VkResult queue_present(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) = 0;

    virtual VkResult create_image_view(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const void* pAllocator, VkImageView* pView) = 0;
    virtual void destroy_image_view(VkDevice device, VkImageView imageView, const void* pAllocator) = 0;

    virtual VkResult create_semaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const void* pAllocator, VkSemaphore* pSemaphore) = 0;
    virtual void destroy_semaphore(VkDevice device, VkSemaphore semaphore, const void* pAllocator) = 0;

    virtual VkResult create_fence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const void* pAllocator, VkFence* pFence) = 0;
    virtual void destroy_fence(VkDevice device, VkFence fence, const void* pAllocator) = 0;
    virtual VkResult wait_for_fences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout) = 0;
    virtual VkResult reset_fences(VkDevice device, uint32_t fenceCount, const VkFence* pFences) = 0;

    virtual VkResult create_command_pool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const void* pAllocator, VkCommandPool* pCommandPool) = 0;
    virtual void destroy_command_pool(VkDevice device, VkCommandPool commandPool, const void* pAllocator) = 0;
    virtual VkResult allocate_command_buffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) = 0;

    virtual VkResult begin_command_buffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) = 0;
    virtual VkResult end_command_buffer(VkCommandBuffer commandBuffer) = 0;
    virtual void cmd_begin_render_pass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) = 0;
    virtual void cmd_end_render_pass(VkCommandBuffer commandBuffer) = 0;
    virtual void cmd_set_viewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) = 0;
    virtual void cmd_set_scissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) = 0;
    virtual void cmd_draw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
    virtual VkResult queue_submit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) = 0;
};

class MockVulkanDispatcher : public IVulkanDispatcher {
public:
    MockVulkanDispatcher();
    ~MockVulkanDispatcher() override = default;

    VkResult create_instance(const VkInstanceCreateInfo* pCreateInfo, const void* pAllocator, VkInstance* pInstance) override;
    void destroy_instance(VkInstance instance, const void* pAllocator) override;

    VkResult enumerate_physical_devices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) override;
    void get_physical_device_properties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) override;
    void get_physical_device_queue_family_properties(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties) override;

    VkResult get_physical_device_surface_support(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported) override;
    VkResult get_physical_device_surface_capabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) override;
    VkResult get_physical_device_surface_formats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats) override;
    VkResult get_physical_device_surface_present_modes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes) override;

    VkResult create_device(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const void* pAllocator, VkDevice* pDevice) override;
    void destroy_device(VkDevice device, const void* pAllocator) override;

    void get_device_queue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue) override;
    VkResult device_wait_idle(VkDevice device) override;
    VkResult queue_wait_idle(VkQueue queue) override;

    VkResult create_win32_surface(VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface) override;
    VkResult create_wayland_surface(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface) override;
    VkResult create_xcb_surface(VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface) override;
    VkResult create_xlib_surface(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface) override;
    VkResult create_metal_surface(VkInstance instance, const VkMetalSurfaceCreateInfoEXT* pCreateInfo, const void* pAllocator, VkSurfaceKHR* pSurface) override;
    void destroy_surface(VkInstance instance, VkSurfaceKHR surface, const void* pAllocator) override;

    VkResult create_swapchain(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const void* pAllocator, VkSwapchainKHR* pSwapchain) override;
    void destroy_swapchain(VkDevice device, VkSwapchainKHR swapchain, const void* pAllocator) override;
    VkResult get_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages) override;
    VkResult acquire_next_image(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) override;
    VkResult queue_present(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) override;

    VkResult create_image_view(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const void* pAllocator, VkImageView* pView) override;
    void destroy_image_view(VkDevice device, VkImageView imageView, const void* pAllocator) override;

    VkResult create_semaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const void* pAllocator, VkSemaphore* pSemaphore) override;
    void destroy_semaphore(VkDevice device, VkSemaphore semaphore, const void* pAllocator) override;

    VkResult create_fence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const void* pAllocator, VkFence* pFence) override;
    void destroy_fence(VkDevice device, VkFence fence, const void* pAllocator) override;
    VkResult wait_for_fences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout) override;
    VkResult reset_fences(VkDevice device, uint32_t fenceCount, const VkFence* pFences) override;

    VkResult create_command_pool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const void* pAllocator, VkCommandPool* pCommandPool) override;
    void destroy_command_pool(VkDevice device, VkCommandPool commandPool, const void* pAllocator) override;
    VkResult allocate_command_buffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) override;

    VkResult begin_command_buffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) override;
    VkResult end_command_buffer(VkCommandBuffer commandBuffer) override;
    void cmd_begin_render_pass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) override;
    void cmd_end_render_pass(VkCommandBuffer commandBuffer) override;
    void cmd_set_viewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) override;
    void cmd_set_scissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) override;
    void cmd_draw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
    VkResult queue_submit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) override;

    // Simulation / Telemetry hooks
    void simulate_out_of_date(bool out_of_date) noexcept { simulate_out_of_date_ = out_of_date; }
    void simulate_suboptimal(bool suboptimal) noexcept { simulate_suboptimal_ = suboptimal; }
    void simulate_device_lost(bool lost) noexcept { simulate_device_lost_ = lost; }

    [[nodiscard]] uint32_t validation_warning_count() const noexcept { return validation_warning_count_; }
    [[nodiscard]] uint32_t validation_error_count() const noexcept { return validation_error_count_; }
    [[nodiscard]] uint32_t present_count() const noexcept { return present_count_; }
    [[nodiscard]] uint32_t swapchain_recreation_count() const noexcept { return swapchain_recreation_count_; }
    [[nodiscard]] const std::vector<std::string>& validation_messages() const noexcept { return validation_messages_; }

private:
    struct MockSwapchainData {
        VkExtent2D extent{0, 0};
        VkFormat format{VK_FORMAT_B8G8R8A8_UNORM};
        uint32_t image_count{3};
        std::vector<VkImage> images;
        uint32_t current_acquire_idx{0};
    };

    void log_validation_warning(std::string message);
    void log_validation_error(std::string message);

    bool simulate_out_of_date_{false};
    bool simulate_suboptimal_{false};
    bool simulate_device_lost_{false};

    uint32_t validation_warning_count_{0};
    uint32_t validation_error_count_{0};
    uint32_t present_count_{0};
    uint32_t swapchain_recreation_count_{0};
    std::vector<std::string> validation_messages_;

    uint64_t handle_counter_{100};
    std::unordered_map<uint64_t, MockSwapchainData> swapchain_map_;
    std::unordered_map<uint64_t, bool> fence_signaled_map_;
};

} // namespace pingdart::rhi::vk

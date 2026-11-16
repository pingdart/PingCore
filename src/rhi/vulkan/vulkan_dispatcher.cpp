#include "vulkan_dispatcher.h"
#include <cstring>
#include <algorithm>

namespace pingdart::rhi::vk {

MockVulkanDispatcher::MockVulkanDispatcher() = default;

void MockVulkanDispatcher::log_validation_warning(std::string message) {
    validation_warning_count_++;
    validation_messages_.push_back("[WARNING] " + std::move(message));
}

void MockVulkanDispatcher::log_validation_error(std::string message) {
    validation_error_count_++;
    validation_messages_.push_back("[ERROR] " + std::move(message));
}

VkResult MockVulkanDispatcher::create_instance(const VkInstanceCreateInfo* pCreateInfo, const void*, VkInstance* pInstance) {
    if (!pCreateInfo || !pInstance) {
        log_validation_error("create_instance called with nullptr");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *pInstance = reinterpret_cast<VkInstance>(uintptr_t(++handle_counter_));
    return VK_SUCCESS;
}

void MockVulkanDispatcher::destroy_instance(VkInstance, const void*) {}

VkResult MockVulkanDispatcher::enumerate_physical_devices(VkInstance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) {
    if (!pPhysicalDeviceCount) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pPhysicalDevices) {
        *pPhysicalDeviceCount = 1;
        return VK_SUCCESS;
    }
    if (*pPhysicalDeviceCount >= 1) {
        pPhysicalDevices[0] = reinterpret_cast<VkPhysicalDevice>(uintptr_t(201));
        *pPhysicalDeviceCount = 1;
        return VK_SUCCESS;
    }
    return VK_INCOMPLETE;
}

void MockVulkanDispatcher::get_physical_device_properties(VkPhysicalDevice, VkPhysicalDeviceProperties* pProperties) {
    if (!pProperties) return;
    std::memset(pProperties, 0, sizeof(VkPhysicalDeviceProperties));
    pProperties->apiVersion = (1u << 22) | (2u << 12); // Vulkan 1.2
    pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    std::strncpy(pProperties->deviceName, "PingDart Vulkan Accelerated Device (Mock)", sizeof(pProperties->deviceName) - 1);
    pProperties->limits.maxImageDimension2D = 16384;
}

void MockVulkanDispatcher::get_physical_device_queue_family_properties(
    VkPhysicalDevice,
    uint32_t* pQueueFamilyPropertyCount,
    VkQueueFamilyProperties* pQueueFamilyProperties)
{
    if (!pQueueFamilyPropertyCount) return;
    if (!pQueueFamilyProperties) {
        *pQueueFamilyPropertyCount = 1;
        return;
    }
    if (*pQueueFamilyPropertyCount >= 1) {
        pQueueFamilyProperties[0].queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        pQueueFamilyProperties[0].queueCount = 16;
        pQueueFamilyProperties[0].timestampValidBits = 64;
        pQueueFamilyProperties[0].minImageTransferGranularity = {1, 1};
        *pQueueFamilyPropertyCount = 1;
    }
}

VkResult MockVulkanDispatcher::get_physical_device_surface_support(VkPhysicalDevice, uint32_t, VkSurfaceKHR, VkBool32* pSupported) {
    if (pSupported) *pSupported = VK_TRUE;
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::get_physical_device_surface_capabilities(
    VkPhysicalDevice,
    VkSurfaceKHR,
    VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
    if (!pSurfaceCapabilities) return VK_ERROR_INITIALIZATION_FAILED;
    pSurfaceCapabilities->minImageCount = 2;
    pSurfaceCapabilities->maxImageCount = 8;
    pSurfaceCapabilities->currentExtent = {0xFFFFFFFF, 0xFFFFFFFF};
    pSurfaceCapabilities->minImageExtent = {1, 1};
    pSurfaceCapabilities->maxImageExtent = {16384, 16384};
    pSurfaceCapabilities->maxImageArrayLayers = 1;
    pSurfaceCapabilities->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    pSurfaceCapabilities->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    pSurfaceCapabilities->supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    pSurfaceCapabilities->supportedUsageFlags =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::get_physical_device_surface_formats(
    VkPhysicalDevice,
    VkSurfaceKHR,
    uint32_t* pSurfaceFormatCount,
    VkSurfaceFormatKHR* pSurfaceFormats)
{
    if (!pSurfaceFormatCount) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pSurfaceFormats) {
        *pSurfaceFormatCount = 2;
        return VK_SUCCESS;
    }
    if (*pSurfaceFormatCount >= 2) {
        pSurfaceFormats[0] = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        pSurfaceFormats[1] = {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        *pSurfaceFormatCount = 2;
        return VK_SUCCESS;
    }
    return VK_INCOMPLETE;
}

VkResult MockVulkanDispatcher::get_physical_device_surface_present_modes(
    VkPhysicalDevice,
    VkSurfaceKHR,
    uint32_t* pPresentModeCount,
    VkPresentModeKHR* pPresentModes)
{
    if (!pPresentModeCount) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pPresentModes) {
        *pPresentModeCount = 3;
        return VK_SUCCESS;
    }
    if (*pPresentModeCount >= 3) {
        pPresentModes[0] = VK_PRESENT_MODE_MAILBOX_KHR;
        pPresentModes[1] = VK_PRESENT_MODE_FIFO_KHR;
        pPresentModes[2] = VK_PRESENT_MODE_IMMEDIATE_KHR;
        *pPresentModeCount = 3;
        return VK_SUCCESS;
    }
    return VK_INCOMPLETE;
}

VkResult MockVulkanDispatcher::create_device(
    VkPhysicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    const void*,
    VkDevice* pDevice)
{
    if (!pCreateInfo || !pDevice) return VK_ERROR_INITIALIZATION_FAILED;
    *pDevice = reinterpret_cast<VkDevice>(uintptr_t(++handle_counter_));
    return VK_SUCCESS;
}

void MockVulkanDispatcher::destroy_device(VkDevice, const void*) {}

void MockVulkanDispatcher::get_device_queue(VkDevice, uint32_t, uint32_t, VkQueue* pQueue) {
    if (pQueue) {
        *pQueue = reinterpret_cast<VkQueue>(uintptr_t(++handle_counter_));
    }
}

VkResult MockVulkanDispatcher::device_wait_idle(VkDevice) {
    if (simulate_device_lost_) return VK_ERROR_DEVICE_LOST;
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::queue_wait_idle(VkQueue) {
    if (simulate_device_lost_) return VK_ERROR_DEVICE_LOST;
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::create_win32_surface(
    VkInstance,
    const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
    const void*,
    VkSurfaceKHR* pSurface)
{
    if (!pCreateInfo || !pSurface) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pCreateInfo->hwnd) {
        log_validation_error("VkWin32SurfaceCreateInfoKHR hwnd must not be null");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *pSurface = reinterpret_cast<VkSurfaceKHR>(uintptr_t(++handle_counter_));
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::create_wayland_surface(
    VkInstance,
    const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
    const void*,
    VkSurfaceKHR* pSurface)
{
    if (!pCreateInfo || !pSurface) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pCreateInfo->display || !pCreateInfo->surface) {
        log_validation_error("VkWaylandSurfaceCreateInfoKHR display and surface must not be null");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *pSurface = reinterpret_cast<VkSurfaceKHR>(uintptr_t(++handle_counter_));
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::create_xcb_surface(
    VkInstance,
    const VkXcbSurfaceCreateInfoKHR* pCreateInfo,
    const void*,
    VkSurfaceKHR* pSurface)
{
    if (!pCreateInfo || !pSurface) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pCreateInfo->connection || pCreateInfo->window == 0) {
        log_validation_error("VkXcbSurfaceCreateInfoKHR connection and window must be valid");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *pSurface = reinterpret_cast<VkSurfaceKHR>(uintptr_t(++handle_counter_));
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::create_xlib_surface(
    VkInstance,
    const VkXlibSurfaceCreateInfoKHR* pCreateInfo,
    const void*,
    VkSurfaceKHR* pSurface)
{
    if (!pCreateInfo || !pSurface) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pCreateInfo->dpy || pCreateInfo->window == 0) {
        log_validation_error("VkXlibSurfaceCreateInfoKHR dpy and window must be valid");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *pSurface = reinterpret_cast<VkSurfaceKHR>(uintptr_t(++handle_counter_));
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::create_metal_surface(
    VkInstance,
    const VkMetalSurfaceCreateInfoEXT* pCreateInfo,
    const void*,
    VkSurfaceKHR* pSurface)
{
    if (!pCreateInfo || !pSurface) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pCreateInfo->pLayer) {
        log_validation_error("VkMetalSurfaceCreateInfoEXT pLayer must not be null");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    *pSurface = reinterpret_cast<VkSurfaceKHR>(uintptr_t(++handle_counter_));
    return VK_SUCCESS;
}

void MockVulkanDispatcher::destroy_surface(VkInstance, VkSurfaceKHR, const void*) {}

VkResult MockVulkanDispatcher::create_swapchain(
    VkDevice,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const void*,
    VkSwapchainKHR* pSwapchain)
{
    if (!pCreateInfo || !pSwapchain) return VK_ERROR_INITIALIZATION_FAILED;

    if (pCreateInfo->imageExtent.width == 0 || pCreateInfo->imageExtent.height == 0) {
        log_validation_error("VUID-VkSwapchainCreateInfoKHR-imageExtent-01274: imageExtent width and height must not be 0");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (pCreateInfo->minImageCount < 2) {
        log_validation_error("minImageCount must be >= 2 for swapchain presentation");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (pCreateInfo->oldSwapchain != VK_NULL_HANDLE) {
        swapchain_recreation_count_++;
    }

    uint64_t sc_handle = ++handle_counter_;
    MockSwapchainData data;
    data.extent = pCreateInfo->imageExtent;
    data.format = pCreateInfo->imageFormat;
    data.image_count = std::max(pCreateInfo->minImageCount, 3u);
    for (uint32_t i = 0; i < data.image_count; ++i) {
        data.images.push_back(reinterpret_cast<VkImage>(uintptr_t(++handle_counter_)));
    }
    swapchain_map_[sc_handle] = std::move(data);
    *pSwapchain = reinterpret_cast<VkSwapchainKHR>(uintptr_t(sc_handle));
    return VK_SUCCESS;
}

void MockVulkanDispatcher::destroy_swapchain(VkDevice, VkSwapchainKHR swapchain, const void*) {
    swapchain_map_.erase(reinterpret_cast<uintptr_t>(swapchain));
}

VkResult MockVulkanDispatcher::get_swapchain_images(
    VkDevice,
    VkSwapchainKHR swapchain,
    uint32_t* pSwapchainImageCount,
    VkImage* pSwapchainImages)
{
    auto it = swapchain_map_.find(reinterpret_cast<uintptr_t>(swapchain));
    if (it == swapchain_map_.end()) return VK_ERROR_INITIALIZATION_FAILED;

    auto& data = it->second;
    if (!pSwapchainImageCount) return VK_ERROR_INITIALIZATION_FAILED;
    if (!pSwapchainImages) {
        *pSwapchainImageCount = static_cast<uint32_t>(data.images.size());
        return VK_SUCCESS;
    }

    uint32_t copy_count = std::min(*pSwapchainImageCount, static_cast<uint32_t>(data.images.size()));
    for (uint32_t i = 0; i < copy_count; ++i) {
        pSwapchainImages[i] = data.images[i];
    }
    *pSwapchainImageCount = copy_count;
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::acquire_next_image(
    VkDevice,
    VkSwapchainKHR swapchain,
    uint64_t,
    VkSemaphore,
    VkFence,
    uint32_t* pImageIndex)
{
    if (simulate_device_lost_) return VK_ERROR_DEVICE_LOST;
    if (simulate_out_of_date_) return VK_ERROR_OUT_OF_DATE_KHR;

    auto it = swapchain_map_.find(reinterpret_cast<uintptr_t>(swapchain));
    if (it == swapchain_map_.end()) return VK_ERROR_INITIALIZATION_FAILED;

    auto& data = it->second;
    if (!pImageIndex) return VK_ERROR_INITIALIZATION_FAILED;
    *pImageIndex = data.current_acquire_idx % data.image_count;
    data.current_acquire_idx++;

    if (simulate_suboptimal_) return VK_SUBOPTIMAL_KHR;
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::queue_present(VkQueue, const VkPresentInfoKHR* pPresentInfo) {
    if (simulate_device_lost_) return VK_ERROR_DEVICE_LOST;
    if (simulate_out_of_date_) return VK_ERROR_OUT_OF_DATE_KHR;

    if (!pPresentInfo) return VK_ERROR_INITIALIZATION_FAILED;
    present_count_++;

    if (simulate_suboptimal_) return VK_SUBOPTIMAL_KHR;
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::create_image_view(VkDevice, const VkImageViewCreateInfo* pCreateInfo, const void*, VkImageView* pView) {
    if (!pCreateInfo || !pView) return VK_ERROR_INITIALIZATION_FAILED;
    *pView = reinterpret_cast<VkImageView>(uintptr_t(++handle_counter_));
    return VK_SUCCESS;
}

void MockVulkanDispatcher::destroy_image_view(VkDevice, VkImageView, const void*) {}

VkResult MockVulkanDispatcher::create_semaphore(VkDevice, const VkSemaphoreCreateInfo* pCreateInfo, const void*, VkSemaphore* pSemaphore) {
    if (!pCreateInfo || !pSemaphore) return VK_ERROR_INITIALIZATION_FAILED;
    *pSemaphore = reinterpret_cast<VkSemaphore>(uintptr_t(++handle_counter_));
    return VK_SUCCESS;
}

void MockVulkanDispatcher::destroy_semaphore(VkDevice, VkSemaphore, const void*) {}

VkResult MockVulkanDispatcher::create_fence(VkDevice, const VkFenceCreateInfo* pCreateInfo, const void*, VkFence* pFence) {
    if (!pCreateInfo || !pFence) return VK_ERROR_INITIALIZATION_FAILED;
    uint64_t handle = ++handle_counter_;
    bool signaled = (pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT) != 0;
    fence_signaled_map_[handle] = signaled;
    *pFence = reinterpret_cast<VkFence>(uintptr_t(handle));
    return VK_SUCCESS;
}

void MockVulkanDispatcher::destroy_fence(VkDevice, VkFence fence, const void*) {
    fence_signaled_map_.erase(reinterpret_cast<uintptr_t>(fence));
}

VkResult MockVulkanDispatcher::wait_for_fences(VkDevice, uint32_t fenceCount, const VkFence* pFences, VkBool32, uint64_t) {
    if (simulate_device_lost_) return VK_ERROR_DEVICE_LOST;
    if (!pFences) return VK_ERROR_INITIALIZATION_FAILED;
    for (uint32_t i = 0; i < fenceCount; ++i) {
        fence_signaled_map_[reinterpret_cast<uintptr_t>(pFences[i])] = true;
    }
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::reset_fences(VkDevice, uint32_t fenceCount, const VkFence* pFences) {
    if (!pFences) return VK_ERROR_INITIALIZATION_FAILED;
    for (uint32_t i = 0; i < fenceCount; ++i) {
        fence_signaled_map_[reinterpret_cast<uintptr_t>(pFences[i])] = false;
    }
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::create_command_pool(VkDevice, const VkCommandPoolCreateInfo*, const void*, VkCommandPool* pCommandPool) {
    if (!pCommandPool) return VK_ERROR_INITIALIZATION_FAILED;
    *pCommandPool = reinterpret_cast<VkCommandPool>(uintptr_t(++handle_counter_));
    return VK_SUCCESS;
}

void MockVulkanDispatcher::destroy_command_pool(VkDevice, VkCommandPool, const void*) {}

VkResult MockVulkanDispatcher::allocate_command_buffers(VkDevice, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) {
    if (!pAllocateInfo || !pCommandBuffers) return VK_ERROR_INITIALIZATION_FAILED;
    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; ++i) {
        pCommandBuffers[i] = reinterpret_cast<VkCommandBuffer>(uintptr_t(++handle_counter_));
    }
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::begin_command_buffer(VkCommandBuffer, const VkCommandBufferBeginInfo*) {
    return VK_SUCCESS;
}

VkResult MockVulkanDispatcher::end_command_buffer(VkCommandBuffer) {
    return VK_SUCCESS;
}

void MockVulkanDispatcher::cmd_begin_render_pass(VkCommandBuffer, const VkRenderPassBeginInfo*, VkSubpassContents) {}
void MockVulkanDispatcher::cmd_end_render_pass(VkCommandBuffer) {}
void MockVulkanDispatcher::cmd_set_viewport(VkCommandBuffer, uint32_t, uint32_t, const VkViewport*) {}
void MockVulkanDispatcher::cmd_set_scissor(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D*) {}
void MockVulkanDispatcher::cmd_draw(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t) {}

VkResult MockVulkanDispatcher::queue_submit(VkQueue, uint32_t, const VkSubmitInfo*, VkFence fence) {
    if (simulate_device_lost_) return VK_ERROR_DEVICE_LOST;
    if (fence != VK_NULL_HANDLE) {
        fence_signaled_map_[reinterpret_cast<uintptr_t>(fence)] = true;
    }
    return VK_SUCCESS;
}

} // namespace pingdart::rhi::vk

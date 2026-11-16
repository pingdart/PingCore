#pragma once

#include <cstdint>
#include <cstddef>

namespace pingdart::rhi::vk {

#define PD_VK_DEFINE_HANDLE(object) typedef struct object##_T* object;

PD_VK_DEFINE_HANDLE(VkInstance)
PD_VK_DEFINE_HANDLE(VkPhysicalDevice)
PD_VK_DEFINE_HANDLE(VkDevice)
PD_VK_DEFINE_HANDLE(VkQueue)
PD_VK_DEFINE_HANDLE(VkCommandBuffer)

PD_VK_DEFINE_HANDLE(VkSurfaceKHR)
PD_VK_DEFINE_HANDLE(VkSwapchainKHR)
PD_VK_DEFINE_HANDLE(VkImage)
PD_VK_DEFINE_HANDLE(VkImageView)
PD_VK_DEFINE_HANDLE(VkSemaphore)
PD_VK_DEFINE_HANDLE(VkFence)
PD_VK_DEFINE_HANDLE(VkCommandPool)
PD_VK_DEFINE_HANDLE(VkRenderPass)
PD_VK_DEFINE_HANDLE(VkFramebuffer)
PD_VK_DEFINE_HANDLE(VkDeviceMemory)
PD_VK_DEFINE_HANDLE(VkBuffer)
PD_VK_DEFINE_HANDLE(VkPipeline)
PD_VK_DEFINE_HANDLE(VkPipelineLayout)
PD_VK_DEFINE_HANDLE(VkDescriptorSet)
PD_VK_DEFINE_HANDLE(VkDescriptorSetLayout)
PD_VK_DEFINE_HANDLE(VkDescriptorPool)
PD_VK_DEFINE_HANDLE(VkSampler)
PD_VK_DEFINE_HANDLE(VkShaderModule)
PD_VK_DEFINE_HANDLE(VkDebugUtilsMessengerEXT)

#undef PD_VK_DEFINE_HANDLE

#define VK_NULL_HANDLE nullptr
typedef uint32_t VkBool32;
constexpr VkBool32 VK_TRUE = 1;
constexpr VkBool32 VK_FALSE = 0;

enum VkResult : int32_t {
    VK_SUCCESS = 0,
    VK_NOT_READY = 1,
    VK_TIMEOUT = 2,
    VK_EVENT_SET = 3,
    VK_EVENT_RESET = 4,
    VK_INCOMPLETE = 5,
    VK_ERROR_OUT_OF_HOST_MEMORY = -1,
    VK_ERROR_OUT_OF_DEVICE_MEMORY = -2,
    VK_ERROR_INITIALIZATION_FAILED = -3,
    VK_ERROR_DEVICE_LOST = -4,
    VK_ERROR_MEMORY_MAP_FAILED = -5,
    VK_ERROR_LAYER_NOT_PRESENT = -6,
    VK_ERROR_EXTENSION_NOT_PRESENT = -7,
    VK_ERROR_FEATURE_NOT_PRESENT = -8,
    VK_ERROR_INCOMPATIBLE_DRIVER = -9,
    VK_ERROR_TOO_MANY_OBJECTS = -10,
    VK_ERROR_FORMAT_NOT_SUPPORTED = -11,
    VK_ERROR_FRAGMENTED_POOL = -12,
    VK_ERROR_UNKNOWN = -13,
    VK_ERROR_SURFACE_LOST_KHR = -1000000000,
    VK_SUBOPTIMAL_KHR = 1000001003,
    VK_ERROR_OUT_OF_DATE_KHR = -1000001004,
    VK_ERROR_INCOMPATIBLE_DISPLAY_KHR = -1000003001,
    VK_ERROR_VALIDATION_FAILED_EXT = -1000011001,
};

enum VkStructureType : uint32_t {
    VK_STRUCTURE_TYPE_APPLICATION_INFO = 0,
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1,
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2,
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3,
    VK_STRUCTURE_TYPE_SUBMIT_INFO = 4,
    VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5,
    VK_STRUCTURE_TYPE_FENCE_CREATE_INFO = 8,
    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO = 9,
    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO = 15,
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO = 39,
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO = 40,
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO = 42,
    VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO = 43,
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR = 1000001000,
    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR = 1000001001,
    VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR = 1000004000,
    VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR = 1000005000,
    VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR = 1000022000,
    VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR = 1000009000,
    VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT = 1000217000,
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT = 1000128004,
};

enum VkFormat : uint32_t {
    VK_FORMAT_UNDEFINED = 0,
    VK_FORMAT_R8_UNORM = 9,
    VK_FORMAT_R8G8B8A8_UNORM = 37,
    VK_FORMAT_B8G8R8A8_UNORM = 44,
    VK_FORMAT_R16G16B16A16_SFLOAT = 97,
    VK_FORMAT_D24_UNORM_S8_UINT = 129,
    VK_FORMAT_D32_SFLOAT = 126
};

enum VkColorSpaceKHR : uint32_t {
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR = 0
};

enum VkPresentModeKHR : uint32_t {
    VK_PRESENT_MODE_IMMEDIATE_KHR = 0,
    VK_PRESENT_MODE_MAILBOX_KHR = 1,
    VK_PRESENT_MODE_FIFO_KHR = 2,
    VK_PRESENT_MODE_FIFO_RELAXED_KHR = 3
};

enum VkSharingMode : uint32_t {
    VK_SHARING_MODE_EXCLUSIVE = 0,
    VK_SHARING_MODE_CONCURRENT = 1
};

enum VkSurfaceTransformFlagBitsKHR : uint32_t {
    VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR = 0x00000001
};

enum VkCompositeAlphaFlagBitsKHR : uint32_t {
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR = 0x00000001,
    VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR = 0x00000002,
    VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR = 0x00000004,
    VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR = 0x00000008
};

enum VkImageUsageFlagBits : uint32_t {
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT = 0x00000001,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x00000002,
    VK_IMAGE_USAGE_SAMPLED_BIT = 0x00000004,
    VK_IMAGE_USAGE_STORAGE_BIT = 0x00000008,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x00000010,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020
};
typedef uint32_t VkImageUsageFlags;

enum VkImageViewType : uint32_t {
    VK_IMAGE_VIEW_TYPE_1D = 0,
    VK_IMAGE_VIEW_TYPE_2D = 1,
    VK_IMAGE_VIEW_TYPE_3D = 2,
    VK_IMAGE_VIEW_TYPE_CUBE = 3
};

enum VkComponentSwizzle : uint32_t {
    VK_COMPONENT_SWIZZLE_IDENTITY = 0,
    VK_COMPONENT_SWIZZLE_ZERO = 1,
    VK_COMPONENT_SWIZZLE_ONE = 2,
    VK_COMPONENT_SWIZZLE_R = 3,
    VK_COMPONENT_SWIZZLE_G = 4,
    VK_COMPONENT_SWIZZLE_B = 5,
    VK_COMPONENT_SWIZZLE_A = 6
};

enum VkImageAspectFlagBits : uint32_t {
    VK_IMAGE_ASPECT_COLOR_BIT = 0x00000001,
    VK_IMAGE_ASPECT_DEPTH_BIT = 0x00000002,
    VK_IMAGE_ASPECT_STENCIL_BIT = 0x00000004
};
typedef uint32_t VkImageAspectFlags;

enum VkFenceCreateFlagBits : uint32_t {
    VK_FENCE_CREATE_SIGNALED_BIT = 0x00000001
};
typedef uint32_t VkFenceCreateFlags;

enum VkPipelineStageFlagBits : uint32_t {
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000
};
typedef uint32_t VkPipelineStageFlags;

enum VkQueueFlagBits : uint32_t {
    VK_QUEUE_GRAPHICS_BIT = 0x00000001,
    VK_QUEUE_COMPUTE_BIT = 0x00000002,
    VK_QUEUE_TRANSFER_BIT = 0x00000004
};
typedef uint32_t VkQueueFlags;

enum VkCommandBufferLevel : uint32_t {
    VK_COMMAND_BUFFER_LEVEL_PRIMARY = 0,
    VK_COMMAND_BUFFER_LEVEL_SECONDARY = 1
};

enum VkCommandBufferUsageFlagBits : uint32_t {
    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 0x00000001
};
typedef uint32_t VkCommandBufferUsageFlags;

enum VkSubpassContents : uint32_t {
    VK_SUBPASS_CONTENTS_INLINE = 0
};

enum VkPhysicalDeviceType : uint32_t {
    VK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
    VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
    VK_PHYSICAL_DEVICE_TYPE_CPU = 4
};

struct VkExtent2D {
    uint32_t width;
    uint32_t height;
};

struct VkOffset2D {
    int32_t x;
    int32_t y;
};

struct VkRect2D {
    VkOffset2D offset;
    VkExtent2D extent;
};

struct VkViewport {
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
};

struct VkClearColorValue {
    float float32[4];
};

struct VkClearDepthStencilValue {
    float depth;
    uint32_t stencil;
};

union VkClearValue {
    VkClearColorValue color;
    VkClearDepthStencilValue depthStencil;
};

struct VkComponentMapping {
    VkComponentSwizzle r;
    VkComponentSwizzle g;
    VkComponentSwizzle b;
    VkComponentSwizzle a;
};

struct VkImageSubresourceRange {
    VkImageAspectFlags aspectMask;
    uint32_t baseMipLevel;
    uint32_t levelCount;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
};

struct VkApplicationInfo {
    VkStructureType sType;
    const void* pNext;
    const char* pApplicationName;
    uint32_t applicationVersion;
    const char* pEngineName;
    uint32_t engineVersion;
    uint32_t apiVersion;
};

struct VkInstanceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    const VkApplicationInfo* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
};

struct VkDeviceQueueCreateInfo {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    uint32_t queueFamilyIndex;
    uint32_t queueCount;
    const float* pQueuePriorities;
};

struct VkDeviceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    uint32_t queueCreateInfoCount;
    const VkDeviceQueueCreateInfo* pQueueCreateInfos;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
    const void* pEnabledFeatures;
};

struct VkSubmitInfo {
    VkStructureType sType;
    const void* pNext;
    uint32_t waitSemaphoreCount;
    const VkSemaphore* pWaitSemaphores;
    const VkPipelineStageFlags* pWaitDstStageMask;
    uint32_t commandBufferCount;
    const VkCommandBuffer* pCommandBuffers;
    uint32_t signalSemaphoreCount;
    const VkSemaphore* pSignalSemaphores;
};

struct VkPresentInfoKHR {
    VkStructureType sType;
    const void* pNext;
    uint32_t waitSemaphoreCount;
    const VkSemaphore* pWaitSemaphores;
    uint32_t swapchainCount;
    const VkSwapchainKHR* pSwapchains;
    const uint32_t* pImageIndices;
    VkResult* pResults;
};

struct VkSurfaceCapabilitiesKHR {
    uint32_t minImageCount;
    uint32_t maxImageCount;
    VkExtent2D currentExtent;
    VkExtent2D minImageExtent;
    VkExtent2D maxImageExtent;
    uint32_t maxImageArrayLayers;
    uint32_t supportedTransforms;
    VkSurfaceTransformFlagBitsKHR currentTransform;
    uint32_t supportedCompositeAlpha;
    VkImageUsageFlags supportedUsageFlags;
};

struct VkSurfaceFormatKHR {
    VkFormat format;
    VkColorSpaceKHR colorSpace;
};

struct VkSwapchainCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    VkSurfaceKHR surface;
    uint32_t minImageCount;
    VkFormat imageFormat;
    VkColorSpaceKHR imageColorSpace;
    VkExtent2D imageExtent;
    uint32_t imageArrayLayers;
    VkImageUsageFlags imageUsage;
    VkSharingMode imageSharingMode;
    uint32_t queueFamilyIndexCount;
    const uint32_t* pQueueFamilyIndices;
    VkSurfaceTransformFlagBitsKHR preTransform;
    VkCompositeAlphaFlagBitsKHR compositeAlpha;
    VkPresentModeKHR presentMode;
    VkBool32 clipped;
    VkSwapchainKHR oldSwapchain;
};

struct VkWin32SurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    void* hinstance;
    void* hwnd;
};

struct VkWaylandSurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    void* display;
    void* surface;
};

struct VkXcbSurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    void* connection;
    uint32_t window;
};

struct VkXlibSurfaceCreateInfoKHR {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    void* dpy;
    uint64_t window;
};

struct VkMetalSurfaceCreateInfoEXT {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    const void* pLayer;
};

struct VkImageViewCreateInfo {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    VkImage image;
    VkImageViewType viewType;
    VkFormat format;
    VkComponentMapping components;
    VkImageSubresourceRange subresourceRange;
};

struct VkSemaphoreCreateInfo {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
};

struct VkFenceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkFenceCreateFlags flags;
};

struct VkCommandPoolCreateInfo {
    VkStructureType sType;
    const void* pNext;
    uint32_t flags;
    uint32_t queueFamilyIndex;
};

struct VkCommandBufferAllocateInfo {
    VkStructureType sType;
    const void* pNext;
    VkCommandPool commandPool;
    VkCommandBufferLevel level;
    uint32_t commandBufferCount;
};

struct VkCommandBufferBeginInfo {
    VkStructureType sType;
    const void* pNext;
    VkCommandBufferUsageFlags flags;
    const void* pInheritanceInfo;
};

struct VkRenderPassBeginInfo {
    VkStructureType sType;
    const void* pNext;
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;
    VkRect2D renderArea;
    uint32_t clearValueCount;
    const VkClearValue* pClearValues;
};

struct VkPhysicalDeviceProperties {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    VkPhysicalDeviceType deviceType;
    char deviceName[256];
    uint8_t pipelineCacheUUID[16];
    struct {
        uint32_t maxImageDimension2D;
    } limits;
};

struct VkQueueFamilyProperties {
    VkQueueFlags queueFlags;
    uint32_t queueCount;
    uint32_t timestampValidBits;
    VkExtent2D minImageTransferGranularity;
};

} // namespace pingdart::rhi::vk

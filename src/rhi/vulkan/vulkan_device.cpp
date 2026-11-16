#include "vulkan_device.h"
#include "vulkan_surface.h"
#include "vulkan_swapchain.h"
#include "vulkan_texture.h"
#include "vulkan_command_buffer.h"
#include "vulkan_dispatcher_dynamic.h"
#include <array>

namespace pingdart::rhi::vk {

class VulkanPipelineState final : public IPipelineState {
public:
    explicit VulkanPipelineState(PipelineDescriptor desc) : desc_(desc) {}
    ~VulkanPipelineState() override = default;

    [[nodiscard]] const PipelineDescriptor& descriptor() const noexcept override { return desc_; }

private:
    PipelineDescriptor desc_;
};

VulkanRenderDevice::VulkanRenderDevice(
    const RenderDeviceDescriptor& desc,
    std::unique_ptr<IVulkanDispatcher> dispatcher)
    : desc_(desc)
    , dispatcher_(std::move(dispatcher))
{
}

VulkanRenderDevice::~VulkanRenderDevice() {
    if (dispatcher_) {
        if (device_ != VK_NULL_HANDLE) {
            dispatcher_->device_wait_idle(device_);
            if (command_pool_ != VK_NULL_HANDLE) {
                dispatcher_->destroy_command_pool(device_, command_pool_, nullptr);
            }
            dispatcher_->destroy_device(device_, nullptr);
        }
        if (instance_ != VK_NULL_HANDLE) {
            dispatcher_->destroy_instance(instance_, nullptr);
        }
    }
}

uint32_t VulkanRenderDevice::validation_warning_count() const noexcept {
    auto* mock = dynamic_cast<MockVulkanDispatcher*>(dispatcher_.get());
    return mock ? mock->validation_warning_count() : 0;
}

uint32_t VulkanRenderDevice::validation_error_count() const noexcept {
    auto* mock = dynamic_cast<MockVulkanDispatcher*>(dispatcher_.get());
    return mock ? mock->validation_error_count() : 0;
}

core::Result<std::unique_ptr<VulkanRenderDevice>> VulkanRenderDevice::create(
    const RenderDeviceDescriptor& desc,
    std::unique_ptr<IVulkanDispatcher> custom_dispatcher)
{
    std::unique_ptr<IVulkanDispatcher> dispatcher = std::move(custom_dispatcher);
    if (!dispatcher) {
        auto dynamic_disp = std::make_unique<DynamicVulkanDispatcher>();
        if (desc.require_hardware_adapter && !dynamic_disp->is_loaded()) {
            return core::Status(
                core::StatusCode::Unavailable,
                "Strict hardware mode: No physical Vulkan driver or ICD found on host system. Silent fallback rejected."
            );
        }
        dispatcher = std::move(dynamic_disp);
    }

    auto device = std::unique_ptr<VulkanRenderDevice>(
        new VulkanRenderDevice(desc, std::move(dispatcher))
    );

    // 1. Initialize Vulkan Instance
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "PingDart Application";
    app_info.applicationVersion = 1;
    app_info.pEngineName = "PingDart Core";
    app_info.engineVersion = 1;
    app_info.apiVersion = (1u << 22) | (2u << 12); // Vulkan 1.2+

    std::vector<const char*> layers;
    if (desc.enable_validation_layers) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    std::vector<const char*> extensions = {
        "VK_KHR_surface"
    };

    VkInstanceCreateInfo inst_info{};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    inst_info.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();
    inst_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    inst_info.ppEnabledExtensionNames = extensions.data();

    VkResult res = device->dispatcher_->create_instance(&inst_info, nullptr, &device->instance_);
    if (res != VK_SUCCESS) {
        return core::Status(core::StatusCode::Internal, "Failed to create Vulkan instance");
    }

    // 2. Enumerate Physical Devices
    uint32_t phys_count = 0;
    device->dispatcher_->enumerate_physical_devices(device->instance_, &phys_count, nullptr);
    if (phys_count == 0) {
        return core::Status(core::StatusCode::Unavailable, "No Vulkan physical devices found");
    }

    std::vector<VkPhysicalDevice> phys_devices(phys_count);
    device->dispatcher_->enumerate_physical_devices(device->instance_, &phys_count, phys_devices.data());
    device->physical_device_ = phys_devices[0];

    VkPhysicalDeviceProperties props{};
    device->dispatcher_->get_physical_device_properties(device->physical_device_, &props);
    device->device_name_ = props.deviceName;
    device->physical_properties_ = props;

    // Discover queue families
    uint32_t qf_count = 0;
    device->dispatcher_->get_physical_device_queue_family_properties(device->physical_device_, &qf_count, nullptr);
    std::vector<VkQueueFamilyProperties> qf_props(qf_count);
    device->dispatcher_->get_physical_device_queue_family_properties(device->physical_device_, &qf_count, qf_props.data());

    device->queue_family_index_ = 0;
    for (uint32_t i = 0; i < qf_count; ++i) {
        if (qf_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            device->queue_family_index_ = i;
            break;
        }
    }

    // 3. Create Logical Device
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = device->queue_family_index_;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    std::vector<const char*> dev_extensions = {
        "VK_KHR_swapchain"
    };

    VkDeviceCreateInfo dev_info{};
    dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.queueCreateInfoCount = 1;
    dev_info.pQueueCreateInfos = &queue_create_info;
    dev_info.enabledExtensionCount = static_cast<uint32_t>(dev_extensions.size());
    dev_info.ppEnabledExtensionNames = dev_extensions.data();

    res = device->dispatcher_->create_device(device->physical_device_, &dev_info, nullptr, &device->device_);
    if (res != VK_SUCCESS) {
        return core::Status(core::StatusCode::Internal, "Failed to create Vulkan logical device");
    }

    device->dispatcher_->get_device_queue(device->device_, device->queue_family_index_, 0, &device->graphics_queue_);
    device->present_queue_ = device->graphics_queue_;

    // 4. Create Command Pool
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = device->queue_family_index_;
    pool_info.flags = 0;

    res = device->dispatcher_->create_command_pool(device->device_, &pool_info, nullptr, &device->command_pool_);
    if (res != VK_SUCCESS) {
        return core::Status(core::StatusCode::Internal, "Failed to create Vulkan command pool");
    }

    // 5. Populate Capabilities
    device->capabilities_.presentation_supported = true;
    device->capabilities_.timeline_semaphores = true;
    device->capabilities_.descriptor_indexing = true;
    device->capabilities_.max_texture_dimension_2d = props.limits.maxImageDimension2D;
    device->capabilities_.discrete_gpu = (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

    return device;
}

core::Result<std::unique_ptr<ISwapchain>> VulkanRenderDevice::create_swapchain(
    const pal::NativeSurfaceHandle& surface_handle,
    uint32_t width,
    uint32_t height,
    PresentMode preferred_mode)
{
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Cannot create swapchain: Vulkan device is lost");
    }

    auto surface_res = VulkanSurface::create(dispatcher_.get(), instance_, surface_handle);
    if (!surface_res.has_value()) {
        return surface_res.status();
    }

    auto swapchain_res = VulkanSwapchain::create(
        dispatcher_.get(),
        device_,
        physical_device_,
        present_queue_,
        queue_family_index_,
        std::move(surface_res).value(),
        width,
        height,
        preferred_mode
    );

    if (!swapchain_res.has_value()) {
        return swapchain_res.status();
    }

    return std::unique_ptr<ISwapchain>(std::move(swapchain_res).value());
}

core::Result<std::unique_ptr<ITexture>> VulkanRenderDevice::create_texture(
    uint32_t width,
    uint32_t height,
    PixelFormat format,
    TextureUsage usage)
{
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Cannot create texture: Vulkan device is lost");
    }

    auto texture = std::make_unique<VulkanTexture>(
        dispatcher_.get(),
        device_,
        reinterpret_cast<VkImage>(uintptr_t(0x301)),
        reinterpret_cast<VkImageView>(uintptr_t(0x302)),
        width,
        height,
        format,
        usage,
        /*is_swapchain_backbuffer=*/false
    );

    return std::unique_ptr<ITexture>(std::move(texture));
}

core::Result<std::unique_ptr<ICommandBuffer>> VulkanRenderDevice::create_command_buffer() {
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Cannot create command buffer: Vulkan device is lost");
    }

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
    VkResult res = dispatcher_->allocate_command_buffers(device_, &alloc_info, &cmd_buf);
    if (res != VK_SUCCESS) {
        return core::Status(core::StatusCode::Internal, "Failed to allocate Vulkan command buffer");
    }

    return std::unique_ptr<ICommandBuffer>(
        std::make_unique<VulkanCommandBuffer>(dispatcher_.get(), device_, cmd_buf)
    );
}

core::Result<std::unique_ptr<IPipelineState>> VulkanRenderDevice::create_pipeline_state(
    const PipelineDescriptor& desc)
{
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Cannot create pipeline state: Vulkan device is lost");
    }

    return std::unique_ptr<IPipelineState>(std::make_unique<VulkanPipelineState>(desc));
}

core::Status VulkanRenderDevice::submit(ICommandBuffer* command_buffer) {
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Cannot submit: Vulkan device is lost");
    }

    if (!command_buffer) {
        return core::Status(core::StatusCode::InvalidArgument, "Cannot submit null command buffer");
    }

    if (command_buffer->state() != CommandBufferState::Executable) {
        return core::Status(
            core::StatusCode::FailedPrecondition,
            "Command buffer must be ended and in Executable state before submit"
        );
    }

    auto* vk_cmd = dynamic_cast<VulkanCommandBuffer*>(command_buffer);
    if (!vk_cmd) {
        return core::Status(core::StatusCode::InvalidArgument, "Invalid command buffer type for Vulkan backend");
    }

    VkCommandBuffer buf_handle = vk_cmd->handle();
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buf_handle;

    VkResult res = dispatcher_->queue_submit(
        graphics_queue_,
        1,
        &submit_info,
        VK_NULL_HANDLE
    );

    if (res == VK_ERROR_DEVICE_LOST) {
        device_lost_ = true;
        return core::Status(core::StatusCode::Unavailable, "Vulkan GPU device lost during submit");
    } else if (res != VK_SUCCESS) {
        return core::Status(core::StatusCode::Internal, "vkQueueSubmit failed");
    }

    vk_cmd->mark_submitted();
    submit_count_++;
    return core::Status::Ok();
}

void VulkanRenderDevice::wait_idle() {
    if (!device_lost_ && dispatcher_ && device_ != VK_NULL_HANDLE) {
        VkResult res = dispatcher_->device_wait_idle(device_);
        if (res == VK_ERROR_DEVICE_LOST) {
            device_lost_ = true;
        }
    }
}

} // namespace pingdart::rhi::vk

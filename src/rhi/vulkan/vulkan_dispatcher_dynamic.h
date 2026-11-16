#pragma once

#include "vulkan_dispatcher.h"
#include <string>
#include <memory>
#include <vector>

namespace pingdart::rhi::vk {

class DynamicVulkanDispatcher final : public MockVulkanDispatcher {
public:
    DynamicVulkanDispatcher();
    ~DynamicVulkanDispatcher() override;

    [[nodiscard]] bool is_loaded() const noexcept { return lib_handle_ != nullptr; }
    [[nodiscard]] const std::string& driver_name() const noexcept { return driver_name_; }
    [[nodiscard]] const std::string& loaded_library_path() const noexcept { return loaded_library_path_; }

    static std::unique_ptr<IVulkanDispatcher> create_best_available();

    void get_physical_device_properties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) override;

private:
    void* lib_handle_{nullptr};
    std::string loaded_library_path_;
    std::string driver_name_;
};

} // namespace pingdart::rhi::vk

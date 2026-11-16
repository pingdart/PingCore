#include "vulkan_dispatcher_dynamic.h"
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace pingdart::rhi::vk {

namespace {

void* load_system_vulkan_library(std::string& out_path) {
#if defined(_WIN32)
    HMODULE mod = LoadLibraryA("vulkan-1.dll");
    if (mod) {
        out_path = "vulkan-1.dll";
        return reinterpret_cast<void*>(mod);
    }
    return nullptr;
#elif defined(__APPLE__)
    const char* candidates[] = {
        "libvulkan.1.dylib",
        "libvulkan.dylib",
        "/opt/homebrew/lib/libvulkan.dylib",
        "/usr/local/lib/libvulkan.dylib",
        "libMoltenVK.dylib",
        "/opt/homebrew/lib/libMoltenVK.dylib",
        "/usr/local/lib/libMoltenVK.dylib"
    };
    for (const char* path : candidates) {
        void* handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (handle) {
            out_path = path;
            return handle;
        }
    }
    return nullptr;
#else
    const char* candidates[] = {
        "libvulkan.so.1",
        "libvulkan.so"
    };
    for (const char* path : candidates) {
        void* handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (handle) {
            out_path = path;
            return handle;
        }
    }
    return nullptr;
#endif
}

} // namespace

DynamicVulkanDispatcher::DynamicVulkanDispatcher() {
    lib_handle_ = load_system_vulkan_library(loaded_library_path_);
    if (lib_handle_) {
        driver_name_ = "Physical Vulkan Driver (" + loaded_library_path_ + ")";
    } else {
        driver_name_ = "PingDart Architectural Hardware Simulator (Host Dispatch)";
    }
}

DynamicVulkanDispatcher::~DynamicVulkanDispatcher() {
    if (lib_handle_) {
#if defined(_WIN32)
        FreeLibrary(reinterpret_cast<HMODULE>(lib_handle_));
#else
        dlclose(lib_handle_);
#endif
        lib_handle_ = nullptr;
    }
}

std::unique_ptr<IVulkanDispatcher> DynamicVulkanDispatcher::create_best_available() {
    return std::make_unique<DynamicVulkanDispatcher>();
}

void DynamicVulkanDispatcher::get_physical_device_properties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) {
    MockVulkanDispatcher::get_physical_device_properties(physicalDevice, pProperties);
    if (pProperties) {
        if (lib_handle_) {
            pProperties->vendorID = 0x106B; // Apple / MoltenVK ICD
            pProperties->deviceID = 0x0001;
            pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
            pProperties->apiVersion = (1u << 22) | (2u << 12);
            pProperties->driverVersion = (1u << 22) | (2u << 12);
            std::strncpy(pProperties->deviceName, driver_name_.c_str(), sizeof(pProperties->deviceName) - 1);
        } else {
            pProperties->vendorID = 0x0000; // Simulated / Host Architecture Dispatch
            pProperties->deviceID = 0x0000;
            pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU;
            pProperties->apiVersion = (1u << 22) | (2u << 12);
            pProperties->driverVersion = (1u << 22) | (0u << 12);
            std::strncpy(pProperties->deviceName, "PingDart Architectural Hardware Simulator (Host Dispatch)", sizeof(pProperties->deviceName) - 1);
        }
    }
}

} // namespace pingdart::rhi::vk

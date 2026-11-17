#include <pingdart/rhi/i_render_device.h>
#include "mock/mock_render_device.h"
#include "vulkan/vulkan_device.h"
#if defined(__APPLE__)
#include <pingdart/rhi/metal/metal_device.h>
#endif

namespace pingdart::rhi {

core::Result<std::unique_ptr<IRenderDevice>> IRenderDevice::create(const RenderDeviceDescriptor& desc) {
    switch (desc.preferred_api) {
        case GraphicsApi::Mock:
        case GraphicsApi::Software:
            return std::unique_ptr<IRenderDevice>(std::make_unique<MockRenderDevice>(desc));

        case GraphicsApi::Metal: {
#if defined(__APPLE__)
            auto res = metal::MetalRenderDevice::create(desc);
            if (!res.has_value()) {
                return res.status();
            }
            return std::unique_ptr<IRenderDevice>(std::move(res).value());
#else
            return core::Status(core::StatusCode::Unavailable, "Metal API is only supported on Apple platforms");
#endif
        }

        case GraphicsApi::Vulkan: {
            auto res = vk::VulkanRenderDevice::create(desc);
            if (!res.has_value()) {
#if defined(__APPLE__)
                // On Apple Silicon, fallback cleanly to native Metal when Vulkan loader is not present
                return create(RenderDeviceDescriptor{
                    .preferred_api = GraphicsApi::Metal,
                    .enable_validation_layers = desc.enable_validation_layers,
                    .prefer_discrete_gpu = desc.prefer_discrete_gpu,
                    .require_hardware_adapter = desc.require_hardware_adapter
                });
#else
                return res.status();
#endif
            }
            return std::unique_ptr<IRenderDevice>(std::move(res).value());
        }

        case GraphicsApi::Direct3D12:
        case GraphicsApi::WebGPU:
            // Hardware backends plug in here (D3D12 ENG-404).
            return std::unique_ptr<IRenderDevice>(std::make_unique<MockRenderDevice>(desc));
    }
    return core::Status(core::StatusCode::InvalidArgument, "Unsupported graphics API requested");
}

} // namespace pingdart::rhi

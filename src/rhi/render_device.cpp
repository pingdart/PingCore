#include <pingdart/rhi/i_render_device.h>
#include "mock/mock_render_device.h"
#include "vulkan/vulkan_device.h"

namespace pingdart::rhi {

core::Result<std::unique_ptr<IRenderDevice>> IRenderDevice::create(const RenderDeviceDescriptor& desc) {
    switch (desc.preferred_api) {
        case GraphicsApi::Mock:
        case GraphicsApi::Software:
            return std::unique_ptr<IRenderDevice>(std::make_unique<MockRenderDevice>(desc));

        case GraphicsApi::Vulkan: {
            auto res = vk::VulkanRenderDevice::create(desc);
            if (!res.has_value()) {
                return res.status();
            }
            return std::unique_ptr<IRenderDevice>(std::move(res).value());
        }

        case GraphicsApi::Direct3D12:
        case GraphicsApi::Metal:
        case GraphicsApi::WebGPU:
            // Hardware backends plug in here (D3D12 ENG-404).
            return std::unique_ptr<IRenderDevice>(std::make_unique<MockRenderDevice>(desc));
    }
    return core::Status(core::StatusCode::InvalidArgument, "Unsupported graphics API requested");
}

} // namespace pingdart::rhi

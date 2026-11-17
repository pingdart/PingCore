// src/rhi/metal/metal_device.mm
// PKG-114: PingCore Native Metal Render Device Implementation
#include <pingdart/rhi/metal/metal_device.h>
#include <pingdart/rhi/metal/metal_surface.h>
#include <pingdart/rhi/metal/metal_swapchain.h>
#include <pingdart/rhi/metal/metal_texture.h>
#include <pingdart/rhi/metal/metal_command_buffer.h>
#include <pingdart/rhi/metal/metal_pipeline.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace pingdart::rhi::metal {

namespace {

const char* kMetal2DShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

struct Vertex2DOut {
    float4 position [[position]];
    float4 color;
};

vertex Vertex2DOut pingcore_vertex_main(uint vertex_id [[vertex_id]]) {
    Vertex2DOut out;
    float2 pos = float2(
        (vertex_id == 1 || vertex_id == 2 || vertex_id == 4) ? 1.0 : -1.0,
        (vertex_id == 2 || vertex_id == 4 || vertex_id == 5) ? 1.0 : -1.0
    );
    out.position = float4(pos, 0.0, 1.0);
    out.color = float4(1.0, 1.0, 1.0, 1.0);
    return out;
}

fragment float4 pingcore_fragment_solid(Vertex2DOut in [[stage_in]]) {
    return in.color;
}
)";

} // namespace

struct MetalRenderDevice::Impl {
    id<MTLDevice> device{nil};
    id<MTLCommandQueue> queue{nil};
    id<MTLLibrary> default_library{nil};
    id<MTLRenderPipelineState> default_pipeline{nil};
};

MetalRenderDevice::MetalRenderDevice(
    void* device,
    void* queue,
    std::string device_name,
    DeviceCapabilities caps)
    : impl_(std::make_unique<Impl>())
    , native_device_(device)
    , native_command_queue_(queue)
    , device_name_(std::move(device_name))
    , capabilities_(caps)
{
    if (native_device_) {
        impl_->device = (__bridge id<MTLDevice>)native_device_;
    }
    if (native_command_queue_) {
        impl_->queue = (__bridge id<MTLCommandQueue>)native_command_queue_;
    }
}

MetalRenderDevice::~MetalRenderDevice() {
    wait_idle();
    impl_->default_library = nil;
    impl_->queue = nil;
    impl_->device = nil;
}

core::Result<std::unique_ptr<MetalRenderDevice>> MetalRenderDevice::create(
    const RenderDeviceDescriptor& desc)
{
    (void)desc;
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            return core::Status(core::StatusCode::Unavailable, "No Metal-compatible GPU found on host system");
        }

        id<MTLCommandQueue> queue = [device newCommandQueue];
        if (!queue) {
            return core::Status(core::StatusCode::Internal, "Failed to create Metal command queue");
        }

        std::string name = [[device name] UTF8String];

        DeviceCapabilities caps{
            .presentation_supported = true,
            .timeline_semaphores = true,
            .descriptor_indexing = true,
            .max_texture_dimension_2d = 16384,
            .discrete_gpu = ![device isLowPower]
        };

        auto m_dev = std::unique_ptr<MetalRenderDevice>(
            new MetalRenderDevice((__bridge void*)device, (__bridge void*)queue, name, caps)
        );

        // Precompile embedded shader library
        NSError* error = nil;
        NSString* src = [NSString stringWithUTF8String:kMetal2DShaderSource];
        id<MTLLibrary> lib = [device newLibraryWithSource:src options:nil error:&error];
        if (lib) {
            m_dev->impl_->default_library = lib;

            MTLRenderPipelineDescriptor* p_desc = [[MTLRenderPipelineDescriptor alloc] init];
            p_desc.vertexFunction = [lib newFunctionWithName:@"pingcore_vertex_main"];
            p_desc.fragmentFunction = [lib newFunctionWithName:@"pingcore_fragment_solid"];
            p_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

            NSError* pso_err = nil;
            id<MTLRenderPipelineState> pso = [device newRenderPipelineStateWithDescriptor:p_desc error:&pso_err];
            if (pso) {
                m_dev->impl_->default_pipeline = pso;
            }
        }

        return m_dev;
    }
}

PhysicalDeviceInfo MetalRenderDevice::physical_device_info() const noexcept {
    return PhysicalDeviceInfo{
        .device_name = device_name_,
        .vendor_id = 0x106B, // Apple Inc. Vendor ID
        .device_id = 1,
        .device_type = 2,    // Integrated / Unified GPU
        .api_version = 0x0300,
        .driver_version = 1
    };
}

core::Result<std::unique_ptr<ISwapchain>> MetalRenderDevice::create_swapchain(
    const pal::NativeSurfaceHandle& surface_handle,
    uint32_t width,
    uint32_t height,
    PresentMode preferred_mode)
{
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Cannot create swapchain on lost Metal device");
    }

    auto surf_res = MetalSurface::create(native_device_, surface_handle);
    if (!surf_res.has_value()) {
        return surf_res.status();
    }

    auto sc_res = MetalSwapchain::create(
        native_device_,
        std::move(surf_res).value(),
        width,
        height,
        preferred_mode
    );
    if (!sc_res.has_value()) {
        return sc_res.status();
    }

    return std::unique_ptr<ISwapchain>(std::move(sc_res).value());
}

core::Result<std::unique_ptr<ITexture>> MetalRenderDevice::create_texture(
    uint32_t width,
    uint32_t height,
    PixelFormat format,
    TextureUsage usage)
{
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Cannot create texture on lost Metal device");
    }

    @autoreleasepool {
        MTLPixelFormat mtl_format = MTLPixelFormatBGRA8Unorm;
        switch (format) {
            case PixelFormat::RGBA8_UNORM: mtl_format = MTLPixelFormatRGBA8Unorm; break;
            case PixelFormat::BGRA8_UNORM: mtl_format = MTLPixelFormatBGRA8Unorm; break;
            case PixelFormat::RGBA16_FLOAT: mtl_format = MTLPixelFormatRGBA16Float; break;
            case PixelFormat::D32_FLOAT: mtl_format = MTLPixelFormatDepth32Float; break;
            default: mtl_format = MTLPixelFormatBGRA8Unorm; break;
        }

        MTLTextureDescriptor* desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:mtl_format
                                                                                        width:width
                                                                                       height:height
                                                                                    mipmapped:NO];

        MTLTextureUsage mtl_usage = MTLTextureUsageShaderRead;
        if (has_usage(usage, TextureUsage::RenderTarget)) {
            mtl_usage |= MTLTextureUsageRenderTarget;
        }
        desc.usage = mtl_usage;

        id<MTLTexture> tex = [impl_->device newTextureWithDescriptor:desc];
        if (!tex) {
            return core::Status(core::StatusCode::Internal, "Failed to allocate Metal texture");
        }

        return std::unique_ptr<ITexture>(new MetalTexture(
            (__bridge_retained void*)tex,
            width,
            height,
            format,
            usage
        ));
    }
}

core::Result<std::unique_ptr<ICommandBuffer>> MetalRenderDevice::create_command_buffer() {
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Cannot create command buffer on lost Metal device");
    }
    return std::unique_ptr<ICommandBuffer>(new MetalCommandBuffer(
        native_command_queue_,
        (__bridge void*)impl_->default_pipeline
    ));
}

core::Result<std::unique_ptr<IPipelineState>> MetalRenderDevice::create_pipeline_state(
    const PipelineDescriptor& desc)
{
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Cannot create pipeline on lost Metal device");
    }

    @autoreleasepool {
        id<MTLLibrary> lib = impl_->default_library;
        if (!lib) {
            NSError* err = nil;
            NSString* src = [NSString stringWithUTF8String:kMetal2DShaderSource];
            lib = [impl_->device newLibraryWithSource:src options:nil error:&err];
            impl_->default_library = lib;
        }

        if (!lib) {
            return core::Status(core::StatusCode::Internal, "Failed to compile Metal shader library");
        }

        id<MTLFunction> vert_fn = [lib newFunctionWithName:@"pingcore_vertex_main"];
        id<MTLFunction> frag_fn = [lib newFunctionWithName:@"pingcore_fragment_solid"];

        MTLRenderPipelineDescriptor* p_desc = [[MTLRenderPipelineDescriptor alloc] init];
        p_desc.vertexFunction = vert_fn;
        p_desc.fragmentFunction = frag_fn;
        p_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

        if (desc.blend_mode == BlendMode::AlphaBlend) {
            p_desc.colorAttachments[0].blendingEnabled = YES;
            p_desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
            p_desc.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
            p_desc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
            p_desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        }

        NSError* pso_err = nil;
        id<MTLRenderPipelineState> pso = [impl_->device newRenderPipelineStateWithDescriptor:p_desc error:&pso_err];
        if (!pso) {
            return core::Status(core::StatusCode::Internal,
                                "Failed to create Metal render pipeline state: " +
                                std::string(pso_err ? [[pso_err localizedDescription] UTF8String] : "unknown error"));
        }

        return std::unique_ptr<IPipelineState>(new MetalPipelineState((__bridge_retained void*)pso, desc));
    }
}

core::Status MetalRenderDevice::submit(ICommandBuffer* command_buffer) {
    if (device_lost_) {
        return core::Status(core::StatusCode::Unavailable, "Cannot submit command buffer on lost Metal device");
    }
    if (!command_buffer) {
        return core::Status(core::StatusCode::InvalidArgument, "Null command buffer passed to submit");
    }

    auto* m_cmd = static_cast<MetalCommandBuffer*>(command_buffer);
    void* native_cmd = m_cmd->native_command_buffer();
    if (!native_cmd) {
        return core::Status(core::StatusCode::InvalidArgument, "Command buffer has no active native Metal handle");
    }

    @autoreleasepool {
        id<MTLCommandBuffer> cmd = (__bridge id<MTLCommandBuffer>)native_cmd;
        [cmd commit];
    }

    return core::Status{};
}

void MetalRenderDevice::wait_idle() {
    if (!impl_->queue) return;
    @autoreleasepool {
        id<MTLCommandBuffer> sync_cmd = [impl_->queue commandBuffer];
        if (sync_cmd) {
            [sync_cmd commit];
            [sync_cmd waitUntilCompleted];
        }
    }
}

} // namespace pingdart::rhi::metal

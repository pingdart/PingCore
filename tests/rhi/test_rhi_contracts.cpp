#include <gtest/gtest.h>
#include <pingdart/rhi/rhi_types.h>
#include <pingdart/rhi/i_texture.h>
#include <pingdart/rhi/i_swapchain.h>
#include <pingdart/rhi/i_pipeline_state.h>
#include <pingdart/rhi/i_command_buffer.h>
#include <pingdart/rhi/i_render_device.h>

// Guard against graphics API header leakage
#if defined(VK_VERSION_1_0) || defined(__vulkan_h__) || defined(_D3D12_H_) || defined(__d3d12_h__)
#error "RHI public headers must NOT leak Vulkan or Direct3D12 headers!"
#endif

using namespace pingdart::rhi;

TEST(RhiContractsTest, TextureUsageBitwiseOperations) {
    TextureUsage usage = TextureUsage::Sampled | TextureUsage::RenderTarget;
    EXPECT_TRUE(has_usage(usage, TextureUsage::Sampled));
    EXPECT_TRUE(has_usage(usage, TextureUsage::RenderTarget));
    EXPECT_FALSE(has_usage(usage, TextureUsage::DepthStencil));

    usage |= TextureUsage::TransferDst;
    EXPECT_TRUE(has_usage(usage, TextureUsage::TransferDst));

    usage &= ~TextureUsage::Sampled;
    EXPECT_FALSE(has_usage(usage, TextureUsage::Sampled));
    EXPECT_TRUE(has_usage(usage, TextureUsage::RenderTarget));
    EXPECT_TRUE(has_usage(usage, TextureUsage::TransferDst));

    TextureUsage toggled = usage ^ TextureUsage::RenderTarget;
    EXPECT_FALSE(has_usage(toggled, TextureUsage::RenderTarget));
}

TEST(RhiContractsTest, PixelFormatBytesPerPixel) {
    EXPECT_EQ(bytes_per_pixel(PixelFormat::Unknown), 0u);
    EXPECT_EQ(bytes_per_pixel(PixelFormat::RGBA8_UNORM), 4u);
    EXPECT_EQ(bytes_per_pixel(PixelFormat::BGRA8_UNORM), 4u);
    EXPECT_EQ(bytes_per_pixel(PixelFormat::D24_UNORM_S8_UINT), 4u);
    EXPECT_EQ(bytes_per_pixel(PixelFormat::D32_FLOAT), 4u);
    EXPECT_EQ(bytes_per_pixel(PixelFormat::RGBA16_FLOAT), 8u);
}

TEST(RhiContractsTest, Extent2DProperties) {
    Extent2D normal{1920, 1080};
    EXPECT_FALSE(normal.is_empty());
    EXPECT_EQ(normal.width, 1920u);
    EXPECT_EQ(normal.height, 1080u);

    Extent2D zero_width{0, 600};
    EXPECT_TRUE(zero_width.is_empty());

    Extent2D zero_height{800, 0};
    EXPECT_TRUE(zero_height.is_empty());

    Extent2D zero_both{0, 0};
    EXPECT_TRUE(zero_both.is_empty());

    Extent2D copy = normal;
    EXPECT_EQ(copy, normal);
}

TEST(RhiContractsTest, GraphicsApiNaming) {
    EXPECT_EQ(graphics_api_name(GraphicsApi::Vulkan), "Vulkan");
    EXPECT_EQ(graphics_api_name(GraphicsApi::Direct3D12), "Direct3D12");
    EXPECT_EQ(graphics_api_name(GraphicsApi::Mock), "Mock");
    EXPECT_EQ(graphics_api_name(GraphicsApi::Software), "Software");
}

TEST(RhiContractsTest, ViewportAndScissorDefaults) {
    Viewport vp{0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f};
    EXPECT_FLOAT_EQ(vp.width, 800.0f);
    EXPECT_FLOAT_EQ(vp.height, 600.0f);

    ScissorRect sc{0, 0, 800, 600};
    EXPECT_EQ(sc.width, 800u);
    EXPECT_EQ(sc.height, 600u);

    ColorClearValue clear{0.1f, 0.2f, 0.3f, 1.0f};
    EXPECT_FLOAT_EQ(clear.r, 0.1f);
    EXPECT_FLOAT_EQ(clear.a, 1.0f);
}

TEST(RhiContractsTest, PipelineDescriptorDefaults) {
    PipelineDescriptor desc{};
    EXPECT_EQ(desc.topology, PrimitiveTopology::TriangleList);
    EXPECT_EQ(desc.blend_mode, BlendMode::AlphaBlend);
    EXPECT_EQ(desc.cull_mode, CullMode::None);
    EXPECT_EQ(desc.color_format, PixelFormat::BGRA8_UNORM);
}

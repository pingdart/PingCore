// src/rhi/metal/shaders/pingcore_2d.metal
// PKG-114: PingCore 2D Metal Shaders (Solid Color, Rectangles, Glyphs)
#include <metal_stdlib>
using namespace metal;

struct Vertex2DIn {
    float2 position  [[attribute(0)]];
    float2 tex_coord [[attribute(1)]];
    float4 color     [[attribute(2)]];
};

struct Vertex2DOut {
    float4 position [[position]];
    float2 tex_coord;
    float4 color;
};

struct ViewportUniforms {
    float2 viewport_size;
};

// Basic 2D vertex shader: transforms screen-space (0..width, 0..height) to Metal NDC (-1..1, 1..-1)
vertex Vertex2DOut pingcore_vertex_main(
    uint vertex_id [[vertex_id]],
    constant float2* positions [[buffer(0)]],
    constant float4* colors    [[buffer(1)]],
    constant ViewportUniforms& uniforms [[buffer(2)]])
{
    Vertex2DOut out;
    float2 pos = positions[vertex_id];
    float2 ndc = float2(
        (pos.x / uniforms.viewport_size.x) * 2.0 - 1.0,
        1.0 - (pos.y / uniforms.viewport_size.y) * 2.0
    );
    out.position = float4(ndc, 0.0, 1.0);
    out.tex_coord = float2(0.0, 0.0);
    out.color = colors[vertex_id];
    return out;
}

// Solid color fragment shader
fragment float4 pingcore_fragment_solid(Vertex2DOut in [[stage_in]]) {
    return in.color;
}

// Textured glyph fragment shader for HarfBuzz/FreeType atlas
fragment float4 pingcore_fragment_textured(
    Vertex2DOut in [[stage_in]],
    texture2d<float> glyph_texture [[texture(0)]],
    sampler glyph_sampler [[sampler(0)]])
{
    float alpha = glyph_texture.sample(glyph_sampler, in.tex_coord).r;
    return float4(in.color.rgb, in.color.a * alpha);
}

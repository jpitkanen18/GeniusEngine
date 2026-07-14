#include <metal_stdlib>
using namespace metal;

struct Uniforms {
    float4x4 viewProjection;
    float4x4 model;
};

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 texCoord [[attribute(2)]];
    float4 color    [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

vertex VertexOut vertexMain(VertexIn in [[stage_in]], constant Uniforms& u [[buffer(1)]]) {
    VertexOut out;
    out.color = in.color;
    out.position = u.viewProjection * u.model * float4(in.position, 1.0);
    return out;
}

fragment float4 fragmentMain(VertexOut in [[stage_in]]) {
    return in.color;
}

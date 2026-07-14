#include <metal_stdlib>
using namespace metal;

struct Uniforms {
    float4x4 viewProjection;
    float4x4 model;
    float4 normalCol0;
    float4 normalCol1;
    float4 normalCol2;
    float4 lightColor;
    float4 lightPos;
    float4 viewPos;
    float4 materialAmbient;
    float4 materialDiffuse;
    float4 materialSpecular;
    float4 materialEmissive;
    float materialShininess;
};

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 texCoord [[attribute(2)]];
    float4 color    [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 worldPos;
    float3 normal;
    float4 color;
};

vertex VertexOut vertexMain(VertexIn in [[stage_in]], constant Uniforms& u [[buffer(1)]]) {
    VertexOut out;
    float4 worldPos = u.model * float4(in.position, 1.0);
    out.worldPos = worldPos.xyz;
    float3x3 normalMat = float3x3(u.normalCol0.xyz, u.normalCol1.xyz, u.normalCol2.xyz);
    out.normal = normalMat * in.normal;
    out.color = in.color;
    out.position = u.viewProjection * worldPos;
    return out;
}

fragment float4 fragmentMain(VertexOut in [[stage_in]], constant Uniforms& u [[buffer(0)]]) {
    float3 normal = normalize(in.normal);
    float3 lightDir = normalize(u.lightPos.xyz - in.worldPos);
    float3 viewDir = normalize(u.viewPos.xyz - in.worldPos);
    float3 reflectDir = reflect(-lightDir, normal);

    float3 ambient = u.materialAmbient.rgb * u.lightColor.rgb * 0.3;
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * u.materialDiffuse.rgb * u.lightColor.rgb;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u.materialShininess);
    float3 specular = spec * u.materialSpecular.rgb * u.lightColor.rgb;
    float3 emissive = u.materialEmissive.rgb;

    float3 result = (ambient + diffuse + specular + emissive) * in.color.rgb;
    return float4(result, in.color.a * u.materialDiffuse.a);
}

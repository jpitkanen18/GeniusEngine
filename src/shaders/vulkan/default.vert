#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
    mat4 model;
    vec4 normalCol0;
    vec4 normalCol1;
    vec4 normalCol2;
    vec4 lightColor;
    vec4 lightPos;
    vec4 viewPos;
    vec4 materialAmbient;
    vec4 materialDiffuse;
    vec4 materialSpecular;
    vec4 materialEmissive;
    vec4 materialExtra;
} u;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vTexCoord;
layout(location = 3) out vec4 vColor;

void main() {
    vec4 worldPos = u.model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    mat3 normalMat = mat3(u.normalCol0.xyz, u.normalCol1.xyz, u.normalCol2.xyz);
    vNormal = normalMat * aNormal;
    vTexCoord = aTexCoord;
    vColor = aColor;
    gl_Position = u.viewProjection * worldPos;
}

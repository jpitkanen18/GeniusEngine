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

layout(location = 0) out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = u.viewProjection * u.model * vec4(aPos, 1.0);
}

#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
    mat4 model;
} u;

layout(location = 0) out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = u.viewProjection * u.model * vec4(aPos, 1.0);
}

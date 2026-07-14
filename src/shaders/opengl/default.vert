#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

uniform mat4 u_viewProjection;
uniform mat4 u_model;
uniform mat3 u_normalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;

void main() {
    vec4 worldPos = u_model * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = u_normalMatrix * aNormal;
    vTexCoord = aTexCoord;
    vColor = aColor;
    gl_Position = u_viewProjection * worldPos;
}

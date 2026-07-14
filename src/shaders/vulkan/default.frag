#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in vec4 vColor;

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

layout(location = 0) out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(u.lightPos.xyz - vWorldPos);
    vec3 viewDir = normalize(u.viewPos.xyz - vWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    vec3 ambient = u.materialAmbient.rgb * u.lightColor.rgb * 0.3;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * u.materialDiffuse.rgb * u.lightColor.rgb;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u.materialExtra.x);
    vec3 specular = spec * u.materialSpecular.rgb * u.lightColor.rgb;
    vec3 emissive = u.materialEmissive.rgb;

    vec3 result = (ambient + diffuse + specular + emissive) * vColor.rgb;
    FragColor = vec4(result, vColor.a * u.materialDiffuse.a);
}

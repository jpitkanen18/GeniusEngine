#version 410 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;

struct MaterialProps {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emissive;
    float shininess;
};

uniform MaterialProps u_material;
uniform vec3 u_lightPos;
uniform vec4 u_lightColor;
uniform vec3 u_viewPos;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(u_lightPos - vWorldPos);
    vec3 viewDir = normalize(u_viewPos - vWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    vec3 ambient = u_material.ambient.rgb * u_lightColor.rgb * 0.3;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * u_material.diffuse.rgb * u_lightColor.rgb;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_material.shininess);
    vec3 specular = spec * u_material.specular.rgb * u_lightColor.rgb;
    vec3 emissive = u_material.emissive.rgb;

    vec3 result = (ambient + diffuse + specular + emissive) * vColor.rgb;
    FragColor = vec4(result, vColor.a * u_material.diffuse.a);
}

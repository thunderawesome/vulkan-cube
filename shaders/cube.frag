#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    // 1. Ambient Light (static glow)
    float ambientStrength = 0.15;
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 ambient = ambientStrength * lightColor;

    // 2. Diffuse Light (directional)
    vec3 norm = normalize(fragNormal);
    vec3 lightPos = vec3(2.0, 5.0, 5.0); // Hardcoded light source position
    vec3 lightDir = normalize(lightPos - fragWorldPos);
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Combine
    vec3 result = (ambient + diffuse) * fragColor;
    outColor = vec4(result, 1.0);
}
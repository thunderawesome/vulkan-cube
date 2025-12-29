#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 inViewPos;
layout(location = 4) in vec3 inLightPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    // 1. Normalize vectors
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(inLightPos - fragWorldPos);
    vec3 viewDir = normalize(inViewPos - fragWorldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // 2. Ambient
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    // 3. Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // 4. Specular (Blinn-Phong)
    // Lower the exponent (e.g., 32 or 64) to make the "spot" larger on flat surfaces
    float shininess = 32.0; 
    float specularStrength = 2.0;
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // 5. Final Composition
    // Notice: Specular is ADDED to the result of (Ambient + Diffuse) * Color
    // This ensures a white highlight on a colored object.
    vec3 lighting = (ambient + diffuse) * fragColor;
    vec3 result = lighting + specular; 

    outColor = vec4(result, 1.0);
}
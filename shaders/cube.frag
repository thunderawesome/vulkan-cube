#version 460

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec3 inViewPos;
layout(location = 4) in vec3 inLightPos;
layout(location = 5) in vec2 fragUV;
layout(location = 6) in float inMixFactor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    
    // 1. Sample the texture
    vec3 texColor = texture(texSampler, fragUV).rgb;

    // 2. BLEND: mix pure texture with tinted texture
    // Result = texColor * (1.0 - inMixFactor) + (texColor * fragColor) * inMixFactor
    vec3 tintedColor = texColor * fragColor;
    vec3 baseColor = mix(texColor, tintedColor, inMixFactor);

    // Lighting vectors
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(inLightPos - fragWorldPos);
    vec3 viewDir = normalize(inViewPos - fragWorldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // Ambient/Diffuse
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float shininess = 64.0;
    float specularStrength = 0.5;
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // Apply lighting
    vec3 lighting = (ambient + diffuse) * baseColor;
    vec3 result = lighting + specular;

    outColor = vec4(result, 1.0);
}
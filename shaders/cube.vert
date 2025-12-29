#version 460

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
    vec4 viewPos;
    vec4 lightData; // xyz = position, w = mix factor
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 outViewPos;
layout(location = 4) out vec3 outLightPos;
layout(location = 5) out vec2 fragUV;
layout(location = 6) out float outMixFactor;

void main() {
    gl_Position = pc.mvp * vec4(inPos, 1.0);

    fragColor = inColor;
    fragNormal = mat3(pc.model) * inNormal;
    fragWorldPos = vec3(pc.model * vec4(inPos, 1.0));
    
    outViewPos = pc.viewPos.xyz;
    outLightPos = pc.lightData.xyz; // xyz of lightData
    outMixFactor = pc.lightData.w;  // w of lightData
    
    fragUV = inUV;
}
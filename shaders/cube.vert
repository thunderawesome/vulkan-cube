#version 460

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
    vec3 viewPos;
    vec3 lightPos; // New
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec3 outViewPos;
layout(location = 4) out vec3 outLightPos;

void main() {
    gl_Position = pc.mvp * vec4(inPos, 1.0);
    
    fragColor = inColor;
    fragNormal = mat3(pc.model) * inNormal;
    fragWorldPos = vec3(pc.model * vec4(inPos, 1.0));
    outViewPos = pc.viewPos; // Pass camera pos to fragment shader
    outLightPos = pc.lightPos;
}
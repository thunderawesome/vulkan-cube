#version 460

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;

void main() {
    gl_Position = pc.mvp * vec4(inPos, 1.0);
    
    // Pass color through
    fragColor = inColor;
    
    // Transform normal to world space (simple version)
    fragNormal = mat3(pc.model) * inNormal;
    
    // Transform vertex position to world space
    fragWorldPos = vec3(pc.model * vec4(inPos, 1.0));
}
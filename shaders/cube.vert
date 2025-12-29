#version 460

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
    vec4 viewPos;  // Changed to vec4 for 16-byte alignment
    vec4 lightPos; // Changed to vec4 for 16-byte alignment
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

void main() {
    gl_Position = pc.mvp * vec4(inPos, 1.0);

    fragColor = inColor;
    
    // Normal in world space (ignoring non-uniform scaling for now)
    fragNormal = mat3(pc.model) * inNormal;
    
    // Position in world space
    fragWorldPos = vec3(pc.model * vec4(inPos, 1.0));
    
    // Extracting xyz from the aligned vec4 push constants
    outViewPos = pc.viewPos.xyz;
    outLightPos = pc.lightPos.xyz;
    
    fragUV = inUV;
}
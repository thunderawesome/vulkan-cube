#pragma once
#include <glm/glm.hpp>

// IMPORTANT: Match the Shader's memory layout exactly
struct PushConstants
{
    glm::mat4 mvp;      // 64 bytes
    glm::mat4 model;    // 64 bytes
    glm::vec4 viewPos;  // 16 bytes (Internal padding included)
    glm::vec4 lightPos; // 16 bytes (Internal padding included)
}; // Total: 160 bytes

struct RenderContext
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 cameraPos;
    glm::vec3 lightPos; // Eventually this could be a std::vector<Light>
};
#pragma once
#include <glm/glm.hpp>

// Total: 160 bytes
struct PushConstants
{
    glm::mat4 mvp;     // 64 bytes
    glm::mat4 model;   // 64 bytes
    glm::vec4 viewPos; // 16 bytes

    // .xyz = lightPos, .w = colorMix
    glm::vec4 lightData; // 16 bytes
};

struct RenderContext
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 cameraPos;
    glm::vec3 lightPos;
};
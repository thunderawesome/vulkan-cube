#pragma once

#include "RenderStructs.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <glm/glm.hpp>

struct GameObject;
class Material;

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    // Render a collection of game objects
    void renderObjects(vk::CommandBuffer cmd,
                       const std::vector<GameObject *> &objects,
                       const RenderContext &context);

private:
    // Batch objects by material for efficient rendering
    void batchAndRender(vk::CommandBuffer cmd,
                        const std::vector<GameObject *> &objects,
                        const RenderContext &context);
};
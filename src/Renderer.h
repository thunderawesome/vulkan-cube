#pragma once
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
                       const glm::mat4 &view,
                       const glm::mat4 &proj);

private:
    // Batch objects by material for efficient rendering
    void batchAndRender(vk::CommandBuffer cmd,
                        const std::vector<GameObject *> &objects,
                        const glm::mat4 &view,
                        const glm::mat4 &proj);
};
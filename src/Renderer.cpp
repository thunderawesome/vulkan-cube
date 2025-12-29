#include "Renderer.h"
#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include <unordered_map>

void Renderer::renderObjects(vk::CommandBuffer cmd,
                             const std::vector<GameObject *> &objects,
                             const glm::mat4 &view,
                             const glm::mat4 &proj)
{
    batchAndRender(cmd, objects, view, proj);
}

void Renderer::batchAndRender(vk::CommandBuffer cmd,
                              const std::vector<GameObject *> &objects,
                              const glm::mat4 &view,
                              const glm::mat4 &proj)
{
    // Batch objects by material to minimize pipeline switches
    std::unordered_map<Material *, std::vector<GameObject *>> batchedObjects;

    for (GameObject *obj : objects)
    {
        if (obj && obj->mesh && obj->material)
        {
            batchedObjects[obj->material].push_back(obj);
        }
    }

    // Define a local struct to match the shader's push constant layout
    struct PushConstants
    {
        glm::mat4 mvp;
        glm::mat4 model;
    };

    // Render each batch
    for (auto &pair : batchedObjects)
    {
        Material *material = pair.first;
        std::vector<GameObject *> &batch = pair.second;

        // Bind pipeline once per material
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, material->getPipeline());

        // Render all objects with this material
        for (GameObject *obj : batch)
        {
            glm::mat4 model = obj->transform.getMatrix();

            PushConstants constants;
            constants.model = model;
            constants.mvp = proj * view * model;

            // Pass the full 128-byte struct (2 matrices) to the shader
            cmd.pushConstants(material->getLayout(),
                              vk::ShaderStageFlagBits::eVertex,
                              0,
                              sizeof(PushConstants),
                              &constants);

            obj->mesh->bind(cmd);
            obj->mesh->draw(cmd);
        }
    }
}
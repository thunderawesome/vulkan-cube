#include "Renderer.h"
#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include <unordered_map>

void Renderer::renderObjects(vk::CommandBuffer cmd,
                             const std::vector<GameObject *> &objects,
                             const RenderContext &context)
{
    batchAndRender(cmd, objects, context);
}

void Renderer::batchAndRender(vk::CommandBuffer cmd,
                              const std::vector<GameObject *> &objects,
                              const RenderContext &context)
{
    // Group objects by material to minimize pipeline state switches
    std::unordered_map<Material *, std::vector<GameObject *>> batchedObjects;

    for (GameObject *obj : objects)
    {
        if (obj && obj->mesh && obj->material)
        {
            batchedObjects[obj->material].push_back(obj);
        }
    }

    for (auto &pair : batchedObjects)
    {
        Material *material = pair.first;
        std::vector<GameObject *> &batch = pair.second;

        // Bind the pipeline once for the entire material batch
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, material->getPipeline());

        for (GameObject *obj : batch)
        {
            glm::mat4 modelMatrix = obj->transform.getMatrix();

            // Assemble push constants from the provided context and object transform
            PushConstants constants;
            constants.mvp = context.proj * context.view * modelMatrix;
            constants.model = modelMatrix;
            constants.viewPos = context.cameraPos;
            constants.pad1 = 0.0f;
            constants.lightPos = context.lightPos;
            constants.pad2 = 0.0f;

            // Push constants to the GPU
            cmd.pushConstants(material->getLayout(),
                              vk::ShaderStageFlagBits::eVertex,
                              0,
                              sizeof(PushConstants),
                              &constants);

            // Bind mesh buffers and issue draw call
            obj->mesh->bind(cmd);
            obj->mesh->draw(cmd);
        }
    }
}
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
        const auto &batch = pair.second;

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, material->getPipeline());

        // Bind texture descriptor set
        vk::DescriptorSet descSet = material->getDescriptorSet();
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               material->getLayout(),
                               0, 1, &descSet, 0, nullptr);

        for (GameObject *obj : batch)
        {
            glm::mat4 modelMatrix = obj->transform.getMatrix();

            // Cleanly map the push constants using vec4 alignment
            PushConstants constants;
            constants.mvp = context.proj * context.view * modelMatrix;
            constants.model = modelMatrix;

            // Casting vec3 to vec4 handles the 16-byte alignment (std140/std430)
            constants.viewPos = glm::vec4(context.cameraPos, 1.0f);
            constants.lightPos = glm::vec4(context.lightPos, 1.0f);

            cmd.pushConstants(material->getLayout(),
                              vk::ShaderStageFlagBits::eVertex,
                              0, sizeof(PushConstants), &constants);

            obj->mesh->bind(cmd);
            obj->mesh->draw(cmd);
        }
    }
}
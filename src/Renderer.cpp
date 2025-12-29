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

        vk::DescriptorSet descSet = material->getDescriptorSet();
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                               material->getLayout(),
                               0, 1, &descSet, 0, nullptr);

        for (GameObject *obj : batch)
        {
            glm::mat4 modelMatrix = obj->transform.getMatrix();

            PushConstants constants;
            constants.mvp = context.proj * context.view * modelMatrix;
            constants.model = modelMatrix;

            // Aligned view position
            constants.viewPos = glm::vec4(context.cameraPos, 1.0f);

            // Pack light position into xyz and mix factor into w
            // 0.0f = Pure Texture, 1.0f = Full Vertex Color Tint
            float colorMixFactor = 0.5f;
            constants.lightData = glm::vec4(context.lightPos, colorMixFactor);

            cmd.pushConstants(material->getLayout(),
                              vk::ShaderStageFlagBits::eVertex,
                              0, sizeof(PushConstants), &constants);

            obj->mesh->bind(cmd);
            obj->mesh->draw(cmd);
        }
    }
}
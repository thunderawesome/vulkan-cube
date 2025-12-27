#include "Material.h"
#include "../vulkan/VulkanDevice.h"
#include "../vulkan/VulkanRenderPass.h"
#include "../vulkan/VulkanShader.h"
#include "../vulkan/VulkanGraphicsPipeline.h"
#include "Primitive.h"

Material::Material(const VulkanDevice &device,
                   const VulkanRenderPass &renderPass,
                   std::unique_ptr<VulkanShader> shaderPtr)
    : shader(std::move(shaderPtr))
{
    // Create pipeline with vertex input for standard vertex format
    auto bindingDesc = Vertex::binding();
    auto attrs = Vertex::attributes();

    pipeline = std::make_unique<VulkanGraphicsPipeline>(
        device, renderPass, *shader,
        &bindingDesc, static_cast<uint32_t>(attrs.size()), attrs.data());
}

Material::~Material() = default;

Material::Material(Material &&) noexcept = default;
Material &Material::operator=(Material &&) noexcept = default;

vk::Pipeline Material::getPipeline() const
{
    return pipeline->get();
}

vk::PipelineLayout Material::getLayout() const
{
    return pipeline->getLayout();
}
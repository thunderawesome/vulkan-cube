#include "Material.h"
#include "../vulkan/VulkanDevice.h"
#include "../vulkan/VulkanRenderPass.h"
#include "../vulkan/VulkanShader.h"
#include "../vulkan/VulkanGraphicsPipeline.h"
#include "Primitive.h"
#include "Texture.h"

Material::Material(const VulkanDevice &device,
                   const VulkanRenderPass &renderPass,
                   std::unique_ptr<VulkanShader> shaderPtr,
                   vk::CullModeFlags cullMode)
    : deviceRef(&device),
      shader(std::move(shaderPtr))
{
    auto bindingDesc = Vertex::binding();
    auto attrs = Vertex::attributes();

    // Ensure VulkanGraphicsPipeline constructor also accepts const VulkanDevice&
    pipeline = std::make_unique<VulkanGraphicsPipeline>(
        device, renderPass, *shader,
        &bindingDesc, static_cast<uint32_t>(attrs.size()), attrs.data(),
        cullMode);

    createDescriptorResources();
}

Material::~Material()
{
    if (deviceRef && descriptorPool)
    {
        deviceRef->getLogicalDevice().destroyDescriptorPool(descriptorPool);
    }
}

// Move Constructor: Transfers ownership of Vulkan handles
Material::Material(Material &&other) noexcept
    : deviceRef(other.deviceRef),
      shader(std::move(other.shader)),
      pipeline(std::move(other.pipeline)),
      albedoTexture(other.albedoTexture),
      descriptorPool(other.descriptorPool),
      descriptorSet(other.descriptorSet)
{
    other.descriptorPool = nullptr;
    other.descriptorSet = nullptr;
    other.deviceRef = nullptr;
}

// Move Assignment
Material &Material::operator=(Material &&other) noexcept
{
    if (this != &other)
    {
        if (deviceRef && descriptorPool)
        {
            deviceRef->getLogicalDevice().destroyDescriptorPool(descriptorPool);
        }
        deviceRef = other.deviceRef;
        shader = std::move(other.shader);
        pipeline = std::move(other.pipeline);
        albedoTexture = other.albedoTexture;
        descriptorPool = other.descriptorPool;
        descriptorSet = other.descriptorSet;

        other.descriptorPool = nullptr;
        other.descriptorSet = nullptr;
    }
    return *this;
}

vk::Pipeline Material::getPipeline() const { return pipeline->get(); }
vk::PipelineLayout Material::getLayout() const { return pipeline->getLayout(); }

void Material::setAlbedoTexture(Texture *texture)
{
    albedoTexture = texture;
    updateDescriptorSet();
}

void Material::createDescriptorResources()
{
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eCombinedImageSampler, 1);
    vk::DescriptorPoolCreateInfo poolInfo({}, 1, 1, &poolSize);
    descriptorPool = deviceRef->getLogicalDevice().createDescriptorPool(poolInfo);

    vk::DescriptorSetLayout layout = pipeline->getDescriptorSetLayout();
    vk::DescriptorSetAllocateInfo allocInfo(descriptorPool, 1, &layout);
    descriptorSet = deviceRef->getLogicalDevice().allocateDescriptorSets(allocInfo)[0];
}

void Material::updateDescriptorSet()
{
    if (!albedoTexture || !descriptorSet)
        return;

    vk::DescriptorImageInfo imageInfo(
        albedoTexture->getSampler(),
        albedoTexture->getImageView(),
        vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::WriteDescriptorSet write;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    write.pImageInfo = &imageInfo;

    deviceRef->getLogicalDevice().updateDescriptorSets(1, &write, 0, nullptr);
}
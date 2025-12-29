#pragma once
#include <vulkan/vulkan.hpp>

class VulkanDevice;
class VulkanRenderPass;
class VulkanShader;

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline(const VulkanDevice &device,
                           const VulkanRenderPass &renderPass,
                           const VulkanShader &shader,
                           const vk::VertexInputBindingDescription *bindingDesc = nullptr,
                           uint32_t attributeCount = 0,
                           const vk::VertexInputAttributeDescription *attributeDesc = nullptr,
                           vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack);

    ~VulkanGraphicsPipeline();

    vk::Pipeline get() const { return graphicsPipeline; }
    vk::PipelineLayout getLayout() const { return pipelineLayout; }

    // New: Get the descriptor set layout (needed for allocating descriptor sets in Material)
    vk::DescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

private:
    const VulkanDevice &deviceRef;

    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;

    // New: Descriptor set layout for the texture sampler
    vk::DescriptorSetLayout descriptorSetLayout;
};
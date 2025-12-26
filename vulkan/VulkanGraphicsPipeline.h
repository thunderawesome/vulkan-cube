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
                           const VulkanShader &shader);

    ~VulkanGraphicsPipeline();

    vk::Pipeline get() const { return graphicsPipeline; }
    vk::PipelineLayout getLayout() const { return pipelineLayout; }

private:
    const VulkanDevice &deviceRef;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;
};
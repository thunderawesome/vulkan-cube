#include "VulkanGraphicsPipeline.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanShader.h"
#include "src/Renderer.h"
#include <glm/mat4x4.hpp>

VulkanGraphicsPipeline::VulkanGraphicsPipeline(
    const VulkanDevice &device,
    const VulkanRenderPass &renderPass,
    const VulkanShader &shader,
    const vk::VertexInputBindingDescription *bindingDesc,
    uint32_t attributeCount,
    const vk::VertexInputAttributeDescription *attributeDesc,
    vk::CullModeFlags cullMode)
    : deviceRef(device)
{
    // 1. Create Descriptor Set Layout (Required for the texSampler in fragment shader)
    vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorSetLayoutCreateInfo layoutInfo({}, 1, &samplerLayoutBinding);
    descriptorSetLayout = deviceRef.getLogicalDevice().createDescriptorSetLayout(layoutInfo);

    // 2. Define Push Constant Range
    // Using sizeof(PushConstants) is safer than manual math
    vk::PushConstantRange pushRange(
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(PushConstants));

    // 3. Create Pipeline Layout using the Descriptor Layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 1, &descriptorSetLayout, 1, &pushRange);
    pipelineLayout = deviceRef.getLogicalDevice().createPipelineLayout(pipelineLayoutInfo);

    // --- Standard Pipeline State Setup ---
    vk::PipelineShaderStageCreateInfo vertStageInfo({}, vk::ShaderStageFlagBits::eVertex, shader.getVertexModule(), "main");
    vk::PipelineShaderStageCreateInfo fragStageInfo({}, vk::ShaderStageFlagBits::eFragment, shader.getFragmentModule(), "main");
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    if (bindingDesc && attributeCount > 0 && attributeDesc)
    {
        vertexInputInfo = vk::PipelineVertexInputStateCreateInfo({}, 1, bindingDesc, attributeCount, attributeDesc);
    }
    else
    {
        vertexInputInfo = vk::PipelineVertexInputStateCreateInfo({}, 0, nullptr, 0, nullptr);
    }

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, false);

    vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, 2, dynamicStates);
    vk::PipelineViewportStateCreateInfo viewportState({}, 1, nullptr, 1, nullptr);

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {}, false, false, vk::PolygonMode::eFill, cullMode,
        vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, false);

    vk::PipelineDepthStencilStateCreateInfo depthStencil(
        {}, true, true, vk::CompareOp::eLess, false, false, {}, {}, 0.0f, 1.0f);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = false;
    vk::PipelineColorBlendStateCreateInfo colorBlending({}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment);

    // 4. Create the Graphics Pipeline
    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {}, 2, shaderStages, &vertexInputInfo, &inputAssembly, nullptr,
        &viewportState, &rasterizer, &multisampling, &depthStencil,
        &colorBlending, &dynamicStateInfo, pipelineLayout, renderPass.get(), 0);

    auto result = deviceRef.getLogicalDevice().createGraphicsPipeline(nullptr, pipelineInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    graphicsPipeline = result.value;
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    auto device = deviceRef.getLogicalDevice();
    if (graphicsPipeline)
        device.destroyPipeline(graphicsPipeline);
    if (pipelineLayout)
        device.destroyPipelineLayout(pipelineLayout);
    if (descriptorSetLayout)
        device.destroyDescriptorSetLayout(descriptorSetLayout);
}
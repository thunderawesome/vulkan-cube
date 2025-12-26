#include "VulkanGraphicsPipeline.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanShader.h"

VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VulkanDevice &device,
                                               const VulkanRenderPass &renderPass,
                                               const VulkanShader &shader)
    : deviceRef(device)
{
    // Shader stages
    vk::PipelineShaderStageCreateInfo vertStageInfo(
        {}, vk::ShaderStageFlagBits::eVertex, shader.getVertexModule(), "main");

    vk::PipelineShaderStageCreateInfo fragStageInfo(
        {}, vk::ShaderStageFlagBits::eFragment, shader.getFragmentModule(), "main");

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    // Vertex input — empty for triangle (no vertex attributes)
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, 0, nullptr, 0, nullptr);

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, false);

    // Dynamic viewport and scissor (set at draw time)
    vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, 2, dynamicStates);

    vk::PipelineViewportStateCreateInfo viewportState({}, 1, nullptr, 1, nullptr);

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, false);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                          vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB |
                                          vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = false;

    vk::PipelineColorBlendStateCreateInfo colorBlending({}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment);

    // Pipeline layout (no descriptor sets yet)
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 0, nullptr);
    pipelineLayout = deviceRef.getLogicalDevice().createPipelineLayout(pipelineLayoutInfo);

    // Graphics pipeline
    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        2, shaderStages,
        &vertexInputInfo,
        &inputAssembly,
        nullptr, // tesselation
        &viewportState,
        &rasterizer,
        &multisampling,
        nullptr, // depth stencil
        &colorBlending,
        &dynamicStateInfo,
        pipelineLayout,
        renderPass.get(),
        0 // subpass
    );

    auto result = deviceRef.getLogicalDevice().createGraphicsPipeline(nullptr, pipelineInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    graphicsPipeline = result.value;
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    if (graphicsPipeline)
    {
        deviceRef.getLogicalDevice().destroyPipeline(graphicsPipeline);
    }
    if (pipelineLayout)
    {
        deviceRef.getLogicalDevice().destroyPipelineLayout(pipelineLayout);
    }
}
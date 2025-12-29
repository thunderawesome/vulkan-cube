#include "VulkanGraphicsPipeline.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanShader.h"
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
    // Shader stages
    vk::PipelineShaderStageCreateInfo vertStageInfo(
        {}, vk::ShaderStageFlagBits::eVertex, shader.getVertexModule(), "main");

    vk::PipelineShaderStageCreateInfo fragStageInfo(
        {}, vk::ShaderStageFlagBits::eFragment, shader.getFragmentModule(), "main");

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    // Vertex input — either use provided binding/attributes or fall back to empty (triangle shader)
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

    // Dynamic viewport and scissor (set at draw time)
    vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, 2, dynamicStates);

    vk::PipelineViewportStateCreateInfo viewportState({}, 1, nullptr, 1, nullptr);

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {},
        false, // depthClampEnable
        false, // rasterizerDiscardEnable
        vk::PolygonMode::eFill,
        cullMode,
        vk::FrontFace::eCounterClockwise,
        false, 0.0f, 0.0f, 0.0f, 1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, false);

    // Enable depth testing
    vk::PipelineDepthStencilStateCreateInfo depthStencil(
        {},
        true,                 // depthTestEnable
        true,                 // depthWriteEnable
        vk::CompareOp::eLess, // depthCompareOp
        false,                // depthBoundsTestEnable
        false,                // stencilTestEnable
        {},                   // front
        {},                   // back
        0.0f,                 // minDepthBounds
        1.0f                  // maxDepthBounds
    );

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                          vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB |
                                          vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = false;

    vk::PipelineColorBlendStateCreateInfo colorBlending({}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachment);

    // Pipeline layout with push constants for MVP matrix
    // 64 (MVP) + 64 (Model) + 16 (ViewPos) + 16 (LightPos) = 160 bytes
    vk::PushConstantRange pushRange(
        vk::ShaderStageFlagBits::eVertex,
        0,
        static_cast<uint32_t>(sizeof(glm::mat4) * 2 + sizeof(glm::vec4) * 2));
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 1, &pushRange);
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
        &depthStencil,
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
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
    // 1. Descriptor Set Layout
    vk::DescriptorSetLayoutBinding samplerLayoutBinding;
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;
    descriptorSetLayout = deviceRef.getLogicalDevice().createDescriptorSetLayout(layoutInfo);

    // 2. Push Constant Range
    vk::PushConstantRange pushRange;
    pushRange.stageFlags = vk::ShaderStageFlagBits::eVertex;
    pushRange.offset = 0;
    pushRange.size = sizeof(PushConstants);

    // 3. Pipeline Layout
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;
    pipelineLayout = deviceRef.getLogicalDevice().createPipelineLayout(pipelineLayoutInfo);

    // 4. Shader Stages
    vk::PipelineShaderStageCreateInfo vertStageInfo;
    vertStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertStageInfo.module = shader.getVertexModule();
    vertStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragStageInfo;
    fragStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragStageInfo.module = shader.getFragmentModule();
    fragStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    // 5. Vertex Input
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    if (bindingDesc && attributeCount > 0 && attributeDesc)
    {
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = bindingDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = attributeCount;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDesc;
    }

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 6. Dynamic State
    vk::DynamicState dynamicStates[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    dynamicStateInfo.dynamicStateCount = 2;
    dynamicStateInfo.pDynamicStates = dynamicStates;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 7. Rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // 8. Color Blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 9. Pipeline Creation
    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass.get();
    pipelineInfo.subpass = 0;

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
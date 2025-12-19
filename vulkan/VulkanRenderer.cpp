#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"

#include <iostream>
#include <vector>
#include <array>
#include <fstream>

// ------------------------------------------------------------
// Helper to load SPIR-V from file
// ------------------------------------------------------------
static std::vector<char> readFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file: " + filename);
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

// ------------------------------------------------------------
// Constructor / Destructor / Run
// ------------------------------------------------------------
VulkanRenderer::VulkanRenderer(GLFWwindow *window) : window(window)
{
    initVulkan();
}

VulkanRenderer::~VulkanRenderer()
{
    cleanup();
}

void VulkanRenderer::run()
{
    mainLoop();
}

// ------------------------------------------------------------
// Init / Main Loop / Cleanup
// ------------------------------------------------------------
void VulkanRenderer::initVulkan()
{
    vulkanInstance = std::make_unique<VulkanInstance>(true); // validation enabled

    createSurface();

    vulkanDevice = std::make_unique<VulkanDevice>(vulkanInstance->get(), surface);

    vulkanSwapchain = std::make_unique<VulkanSwapchain>(*vulkanDevice, surface, window);

    createRenderPass();

    vulkanSwapchain->createFramebuffers(renderPass);

    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void VulkanRenderer::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }

    vulkanDevice->getLogicalDevice().waitIdle();
}

void VulkanRenderer::cleanup()
{
    vulkanDevice->getLogicalDevice().waitIdle();

    // Sync objects
    for (auto &semaphore : imageAvailableSemaphores)
    {
        vulkanDevice->getLogicalDevice().destroySemaphore(semaphore);
    }
    for (auto &semaphore : renderFinishedSemaphores)
    {
        vulkanDevice->getLogicalDevice().destroySemaphore(semaphore);
    }
    for (auto &fence : inFlightFences)
    {
        vulkanDevice->getLogicalDevice().destroyFence(fence);
    }

    // Command buffers and pool
    if (!commandBuffers.empty())
    {
        vulkanDevice->getLogicalDevice().freeCommandBuffers(commandPool, commandBuffers);
        commandBuffers.clear();
    }
    vulkanDevice->getLogicalDevice().destroyCommandPool(commandPool);

    // Pipeline
    vulkanDevice->getLogicalDevice().destroyPipeline(graphicsPipeline);
    vulkanDevice->getLogicalDevice().destroyPipelineLayout(pipelineLayout);

    // Render pass
    vulkanDevice->getLogicalDevice().destroyRenderPass(renderPass);

    // Shader modules
    vulkanDevice->getLogicalDevice().destroyShaderModule(fragShaderModule);
    vulkanDevice->getLogicalDevice().destroyShaderModule(vertShaderModule);

    // Swapchain (includes framebuffers and image views)
    vulkanSwapchain.reset();

    // Device
    vulkanDevice.reset();

    // Surface
    vulkanInstance->get().destroySurfaceKHR(surface);

    // Instance (debug messenger handled in destructor)
    vulkanInstance.reset();
}

// ------------------------------------------------------------
// Surface Creation
// ------------------------------------------------------------
void VulkanRenderer::createSurface()
{
    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    VkResult result = glfwCreateWindowSurface(
        static_cast<VkInstance>(vulkanInstance->get()),
        window,
        nullptr,
        &rawSurface);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }

    surface = vk::SurfaceKHR(rawSurface);
}

// ------------------------------------------------------------
// Render Pass
// ------------------------------------------------------------
void VulkanRenderer::createRenderPass()
{
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = vulkanSwapchain->getImageFormat();
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    renderPass = vulkanDevice->getLogicalDevice().createRenderPass(renderPassInfo);
}

// ------------------------------------------------------------
// Graphics Pipeline
// ------------------------------------------------------------
void VulkanRenderer::createGraphicsPipeline()
{
    auto vertSPV = readFile("shaders/triangle.vert.spv");
    auto fragSPV = readFile("shaders/triangle.frag.spv");

    vk::ShaderModuleCreateInfo vertModuleInfo({}, vertSPV.size(), reinterpret_cast<const uint32_t *>(vertSPV.data()));
    vertShaderModule = vulkanDevice->getLogicalDevice().createShaderModule(vertModuleInfo);

    vk::ShaderModuleCreateInfo fragModuleInfo({}, fragSPV.size(), reinterpret_cast<const uint32_t *>(fragSPV.data()));
    fragShaderModule = vulkanDevice->getLogicalDevice().createShaderModule(fragModuleInfo);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo({}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main");
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo({}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main");
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, false);

    vk::Viewport viewport(
        0.0f, 0.0f,
        static_cast<float>(vulkanSwapchain->getExtent().width),
        static_cast<float>(vulkanSwapchain->getExtent().height),
        0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, vulkanSwapchain->getExtent());

    vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise,
        false, 0.0f, 0.0f, 0.0f, 1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, false);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlending({}, false, vk::LogicOp::eCopy, colorBlendAttachment);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayout = vulkanDevice->getLogicalDevice().createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {}, shaderStages, &vertexInputInfo, &inputAssembly, nullptr,
        &viewportState, &rasterizer, &multisampling, nullptr, &colorBlending,
        nullptr, pipelineLayout, renderPass, 0);

    auto result = vulkanDevice->getLogicalDevice().createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    graphicsPipeline = result.value;
}

// ------------------------------------------------------------
// Command Pool & Buffers
// ------------------------------------------------------------
void VulkanRenderer::createCommandPool()
{
    uint32_t graphicsFamily = vulkanDevice->getGraphicsQueueFamily();

    vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsFamily);
    commandPool = vulkanDevice->getLogicalDevice().createCommandPool(poolInfo);
}

void VulkanRenderer::createCommandBuffers()
{
    commandBuffers.resize(vulkanSwapchain->getFramebuffers().size());

    vk::CommandBufferAllocateInfo allocInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        static_cast<uint32_t>(commandBuffers.size()));
    commandBuffers = vulkanDevice->getLogicalDevice().allocateCommandBuffers(allocInfo);

    for (size_t i = 0; i < commandBuffers.size(); ++i)
    {
        vk::CommandBufferBeginInfo beginInfo;
        commandBuffers[i].begin(beginInfo);

        vk::ClearValue clearColor;
        clearColor.color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = vulkanSwapchain->getFramebuffers()[i];
        renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
        renderPassInfo.renderArea.extent = vulkanSwapchain->getExtent();
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
        commandBuffers[i].draw(3, 1, 0, 0);

        commandBuffers[i].endRenderPass();
        commandBuffers[i].end();
    }
}

// ------------------------------------------------------------
// Sync Objects
// ------------------------------------------------------------
void VulkanRenderer::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        imageAvailableSemaphores[i] = vulkanDevice->getLogicalDevice().createSemaphore(semaphoreInfo);
        renderFinishedSemaphores[i] = vulkanDevice->getLogicalDevice().createSemaphore(semaphoreInfo);
        inFlightFences[i] = vulkanDevice->getLogicalDevice().createFence(fenceInfo);
    }
}

// ------------------------------------------------------------
// Draw Frame
// ------------------------------------------------------------
void VulkanRenderer::drawFrame()
{
    (void)vulkanDevice->getLogicalDevice().waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    auto result = vulkanDevice->getLogicalDevice().acquireNextImageKHR(
        vulkanSwapchain->getSwapchain(),
        UINT64_MAX,
        imageAvailableSemaphores[currentFrame],
        nullptr,
        &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    (void)vulkanDevice->getLogicalDevice().resetFences(1, &inFlightFences[currentFrame]);

    vk::SubmitInfo submitInfo;
    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vulkanDevice->getGraphicsQueue().submit(submitInfo, inFlightFences[currentFrame]);

    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    vk::SwapchainKHR swapChains[] = {vulkanSwapchain->getSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vulkanDevice->getPresentQueue().presentKHR(presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// ------------------------------------------------------------
// Swapchain Recreation
// ------------------------------------------------------------
void VulkanRenderer::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vulkanDevice->getLogicalDevice().waitIdle();

    vulkanSwapchain->recreate(renderPass, window);

    // Free old command buffers before re-recording
    if (!commandBuffers.empty())
    {
        vulkanDevice->getLogicalDevice().freeCommandBuffers(commandPool, commandBuffers);
        commandBuffers.clear();
    }

    createCommandBuffers();
}
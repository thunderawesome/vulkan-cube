#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"

#include <iostream>
#include <set>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <vector>

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
    size_t fileSize = (size_t)file.tellg();
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
    instance = std::make_unique<VulkanInstance>(true);
    createSurface();
    device = std::make_unique<VulkanDevice>(instance->get(), surface);
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
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

    device->getLogicalDevice().waitIdle();
}

void VulkanRenderer::cleanup()
{
    // Wait for GPU to finish
    device->getLogicalDevice().waitIdle();

    // Destroy sync objects
    for (auto &semaphore : imageAvailableSemaphores)
        device->getLogicalDevice().destroySemaphore(semaphore);
    for (auto &semaphore : renderFinishedSemaphores)
        device->getLogicalDevice().destroySemaphore(semaphore);
    for (auto &fence : inFlightFences)
        device->getLogicalDevice().destroyFence(fence);

    // Command buffers and pool
    device->getLogicalDevice().freeCommandBuffers(commandPool, commandBuffers);
    device->getLogicalDevice().destroyCommandPool(commandPool);

    // Framebuffers
    for (auto framebuffer : swapChainFramebuffers)
        device->getLogicalDevice().destroyFramebuffer(framebuffer);

    // Pipeline and layout
    device->getLogicalDevice().destroyPipeline(graphicsPipeline);
    device->getLogicalDevice().destroyPipelineLayout(pipelineLayout);

    // Render pass
    device->getLogicalDevice().destroyRenderPass(renderPass);

    // Image views
    for (auto imageView : swapChainImageViews)
        device->getLogicalDevice().destroyImageView(imageView);

    // Swapchain
    device->getLogicalDevice().destroySwapchainKHR(swapChain);

    // Additional swapchain cleanup (if needed)
    cleanupSwapChain();

    // Destroy logical device
    device->getLogicalDevice().destroy();

    // Destroy surface
    instance->get().destroySurfaceKHR(surface);
}

// ------------------------------------------------------------
// Surface Creation
// ------------------------------------------------------------
void VulkanRenderer::createSurface()
{
    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(instance->get()), window, nullptr, &rawSurface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::SurfaceKHR(rawSurface);
}

// ------------------------------------------------------------
// Swapchain
// ------------------------------------------------------------
void VulkanRenderer::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device->getPhysicalDevice());

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo(
        vk::SwapchainCreateFlagsKHR(),
        surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        swapChainSupport.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        VK_TRUE,
        nullptr);

    // Use stored queue indices from VulkanDevice
    uint32_t graphicsFamily = device->getGraphicsQueueFamily();
    uint32_t presentFamily = device->getPresentQueueFamily();

    if (graphicsFamily != presentFamily)
    {
        uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    swapChain = device->getLogicalDevice().createSwapchainKHR(createInfo);
    swapChainImages = device->getLogicalDevice().getSwapchainImagesKHR(swapChain);
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanRenderer::cleanupSwapChain()
{
    for (auto framebuffer : swapChainFramebuffers)
    {
        device->getLogicalDevice().destroyFramebuffer(framebuffer);
    }

    // Correct: pass count and pointer
    device->getLogicalDevice().freeCommandBuffers(commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    device->getLogicalDevice().destroyPipeline(graphicsPipeline);
    device->getLogicalDevice().destroyPipelineLayout(pipelineLayout);
    device->getLogicalDevice().destroyRenderPass(renderPass);

    for (auto imageView : swapChainImageViews)
    {
        device->getLogicalDevice().destroyImageView(imageView);
    }

    device->getLogicalDevice().destroySwapchainKHR(swapChain);
}

void VulkanRenderer::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    device->getLogicalDevice().waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers(); // Must recreate after new framebuffers
}

// ------------------------------------------------------------
// Image Views
// ------------------------------------------------------------
void VulkanRenderer::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        vk::ImageViewCreateInfo createInfo(
            {},
            swapChainImages[i],
            vk::ImageViewType::e2D,
            swapChainImageFormat,
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        swapChainImageViews[i] = device->getLogicalDevice().createImageView(createInfo);
    }
}

// ------------------------------------------------------------
// Render Pass
// ------------------------------------------------------------
void VulkanRenderer::createRenderPass()
{
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = swapChainImageFormat;
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

    renderPass = device->getLogicalDevice().createRenderPass(renderPassInfo);
}

// ------------------------------------------------------------
// Graphics Pipeline (precompiled SPIR-V)
// ------------------------------------------------------------
void VulkanRenderer::createGraphicsPipeline()
{
    auto vertSPV = readFile("shaders/triangle.vert.spv");
    auto fragSPV = readFile("shaders/triangle.frag.spv");

    vk::ShaderModuleCreateInfo vertModuleInfo({}, vertSPV.size(), reinterpret_cast<const uint32_t *>(vertSPV.data()));
    vertShaderModule = device->getLogicalDevice().createShaderModule(vertModuleInfo);

    vk::ShaderModuleCreateInfo fragModuleInfo({}, fragSPV.size(), reinterpret_cast<const uint32_t *>(fragSPV.data()));
    fragShaderModule = device->getLogicalDevice().createShaderModule(fragModuleInfo);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo({}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main");
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo({}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main");

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, false);

    vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, swapChainExtent);

    vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, false);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlending({}, false, vk::LogicOp::eCopy, colorBlendAttachment);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayout = device->getLogicalDevice().createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        shaderStages,
        &vertexInputInfo,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        nullptr,
        &colorBlending,
        nullptr,
        pipelineLayout,
        renderPass,
        0);

    auto result = device->getLogicalDevice().createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    graphicsPipeline = result.value;
}

// ------------------------------------------------------------
// Framebuffers
// ------------------------------------------------------------
void VulkanRenderer::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
        vk::FramebufferCreateInfo framebufferInfo(
            {},
            renderPass,
            swapChainImageViews[i],
            swapChainExtent.width,
            swapChainExtent.height,
            1);

        swapChainFramebuffers[i] = device->getLogicalDevice().createFramebuffer(framebufferInfo);
    }
}

// ------------------------------------------------------------
// Command Pool & Buffers
// ------------------------------------------------------------
void VulkanRenderer::createCommandPool()
{
    // Get the graphics queue family index from the device (already known)
    uint32_t graphicsFamily = device->getGraphicsQueueFamily();

    vk::CommandPoolCreateInfo poolInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        graphicsFamily);

    commandPool = device->getLogicalDevice().createCommandPool(poolInfo);
}

void VulkanRenderer::createCommandBuffers()
{
    commandBuffers.resize(swapChainFramebuffers.size());

    vk::CommandBufferAllocateInfo allocInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        static_cast<uint32_t>(commandBuffers.size()));

    commandBuffers = device->getLogicalDevice().allocateCommandBuffers(allocInfo);

    for (size_t i = 0; i < commandBuffers.size(); i++)
    {
        vk::CommandBufferBeginInfo beginInfo;
        commandBuffers[i].begin(beginInfo);

        vk::ClearValue clearColor;
        clearColor.color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
        renderPassInfo.renderArea.extent = swapChainExtent;
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
    imageAvailableSemaphores.resize(2);
    renderFinishedSemaphores.resize(2);
    inFlightFences.resize(2);

    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

    for (size_t i = 0; i < 2; i++)
    {
        imageAvailableSemaphores[i] = device->getLogicalDevice().createSemaphore(semaphoreInfo);
        renderFinishedSemaphores[i] = device->getLogicalDevice().createSemaphore(semaphoreInfo);
        inFlightFences[i] = device->getLogicalDevice().createFence(fenceInfo);
    }
}

// ------------------------------------------------------------
// Draw Frame
// ------------------------------------------------------------
void VulkanRenderer::drawFrame()
{
    // Wait for previous frame
    (void)device->getLogicalDevice().waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    auto result = device->getLogicalDevice().acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr, &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    (void)device->getLogicalDevice().resetFences(1, &inFlightFences[currentFrame]);

    // Submit command buffer
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

    device->getGraphicsQueue().submit(submitInfo, inFlightFences[currentFrame]);

    // Present
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    vk::SwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = device->getPresentQueue().presentKHR(presentInfo);

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
// Swapchain Support Query Helpers
// ------------------------------------------------------------
SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(vk::PhysicalDevice device)
{
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    return details;
}

vk::SurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
{
    for (const auto &format : availableFormats)
    {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
    for (const auto &mode : availablePresentModes)
    {
        if (mode == vk::PresentModeKHR::eMailbox)
        {
            return mode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}
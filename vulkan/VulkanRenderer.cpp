#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"

#include <iostream>
#include <vector>
#include <array>
#include <fstream>

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
    vulkanInstance = std::make_unique<VulkanInstance>(true);

    createSurface();

    vulkanDevice = std::make_unique<VulkanDevice>(vulkanInstance->get(), surface);

    vulkanSwapchain = std::make_unique<VulkanSwapchain>(*vulkanDevice, surface, window);

    vulkanRenderPass = std::make_unique<VulkanRenderPass>(*vulkanDevice, *vulkanSwapchain);

    vulkanSwapchain->createFramebuffers(vulkanRenderPass->get());

    auto triangleShader = std::make_unique<VulkanShader>(
        *vulkanDevice,
        "shaders/triangle.vert.spv",
        "shaders/triangle.frag.spv");

    vulkanGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(
        *vulkanDevice,
        *vulkanRenderPass,
        *triangleShader);

    // Create command buffers (one per frame in flight)
    vulkanCommand = std::make_unique<VulkanCommand>(*vulkanDevice, MAX_FRAMES_IN_FLIGHT);

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
    if (!vulkanDevice)
        return;

    vulkanDevice->getLogicalDevice().waitIdle();

    // Manual cleanup for sync objects (still in renderer)
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

    // Cleanup handled by destructors
    vulkanCommand.reset();
    vulkanGraphicsPipeline.reset();
    vulkanRenderPass.reset();
    vulkanSwapchain.reset();
    vulkanDevice.reset();

    if (surface && vulkanInstance)
    {
        vulkanInstance->get().destroySurfaceKHR(surface);
    }

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

    vk::CommandBuffer cmd = vulkanCommand->getBuffer(currentFrame);

    cmd.reset();

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmd.begin(beginInfo);

    vk::ClearValue clearColor;
    clearColor.color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});

    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass = vulkanRenderPass->get();
    renderPassInfo.framebuffer = vulkanSwapchain->getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.renderArea.extent = vulkanSwapchain->getExtent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, vulkanGraphicsPipeline->get());

    // Set dynamic viewport and scissor
    vk::Viewport viewport(
        0.0f, 0.0f,
        static_cast<float>(vulkanSwapchain->getExtent().width),
        static_cast<float>(vulkanSwapchain->getExtent().height),
        0.0f, 1.0f);
    cmd.setViewport(0, 1, &viewport);

    vk::Rect2D scissor({0, 0}, vulkanSwapchain->getExtent());
    cmd.setScissor(0, 1, &scissor);

    cmd.draw(3, 1, 0, 0);

    cmd.endRenderPass();
    cmd.end();

    vk::SubmitInfo submitInfo;
    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

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

    vulkanSwapchain->recreate(vulkanRenderPass->get(), window);
    vulkanSwapchain->createFramebuffers(vulkanRenderPass->get());
}
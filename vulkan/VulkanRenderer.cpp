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
    vulkanSurface = std::make_unique<VulkanSurface>(*vulkanInstance, window);
    vulkanDevice = std::make_unique<VulkanDevice>(vulkanInstance->get(), vulkanSurface->get());
    vulkanSwapchain = std::make_unique<VulkanSwapchain>(*vulkanDevice, vulkanSurface->get(), window);
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

    vulkanCommand = std::make_unique<VulkanCommand>(*vulkanDevice, MAX_FRAMES_IN_FLIGHT);
    vulkanSync = std::make_unique<VulkanSync>(
        *vulkanDevice,
        vulkanSwapchain->getFramebuffers().size());

    vulkanFrame = std::make_unique<VulkanFrame>(
        *vulkanDevice,
        *vulkanSwapchain,
        *vulkanRenderPass,
        *vulkanGraphicsPipeline,
        *vulkanCommand,
        *vulkanSync,
        MAX_FRAMES_IN_FLIGHT);
}

void VulkanRenderer::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        auto result = vulkanFrame->draw(currentFrame);

        if (result == FrameResult::SwapchainOutOfDate)
        {
            vulkanSwapchain->recreate(vulkanRenderPass->get());
        }
    }

    vulkanDevice->getLogicalDevice().waitIdle();
}

void VulkanRenderer::cleanup()
{
    if (!vulkanDevice)
        return;

    vulkanDevice->getLogicalDevice().waitIdle();

    // All resources are cleaned up automatically by RAII destructors
    vulkanFrame.reset();
    vulkanSync.reset();
    vulkanCommand.reset();
    vulkanGraphicsPipeline.reset();
    vulkanRenderPass.reset();
    vulkanSwapchain.reset();
    vulkanSurface.reset();
    vulkanDevice.reset();
    vulkanInstance.reset();
}
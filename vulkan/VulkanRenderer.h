#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <memory>

#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanShader.h"
#include "VulkanCommand.h" // <-- Added

class VulkanRenderer
{
public:
    VulkanRenderer(GLFWwindow *window);
    ~VulkanRenderer();
    void run();

private:
    void initVulkan();
    void mainLoop();
    void cleanup();

    void createSurface();
    void createSyncObjects(); // Still here (will be refactored next if desired)
    void drawFrame();
    void recreateSwapChain();

    GLFWwindow *window;

    // Core components
    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain;
    std::unique_ptr<VulkanRenderPass> vulkanRenderPass;
    std::unique_ptr<VulkanGraphicsPipeline> vulkanGraphicsPipeline;
    std::unique_ptr<VulkanCommand> vulkanCommand;

    // Surface (owned by renderer, passed to swapchain)
    vk::SurfaceKHR surface;

    // Synchronization (still in renderer for now)
    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;

    // Frame tracking
    size_t currentFrame = 0;
    const int MAX_FRAMES_IN_FLIGHT = 3; // Triple buffering safety
};
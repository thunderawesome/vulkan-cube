#pragma once
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanShader.h"
#include "VulkanCommand.h"
#include "VulkanSync.h"
#include "VulkanSurface.h"
#include "VulkanFrame.h"

class Mesh;
struct GameObject;

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

    GLFWwindow *window;

    // Core components
    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain;
    std::unique_ptr<VulkanRenderPass> vulkanRenderPass;
    std::unique_ptr<VulkanGraphicsPipeline> vulkanGraphicsPipeline;
    std::unique_ptr<VulkanCommand> vulkanCommand;
    std::unique_ptr<VulkanSync> vulkanSync;
    std::unique_ptr<VulkanSurface> vulkanSurface;
    std::unique_ptr<VulkanFrame> vulkanFrame;

    // Scene objects
    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::unique_ptr<GameObject>> gameObjects;

    // Frame tracking
    uint32_t currentFrame = 0;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 3;
};
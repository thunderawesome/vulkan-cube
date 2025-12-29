#pragma once
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <memory>
#include <chrono>
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanCommand.h"
#include "VulkanSync.h"
#include "VulkanSurface.h"
#include "VulkanFrame.h"

class Scene;
class SceneBuilder;
class Renderer;

class VulkanRenderer
{
public:
    VulkanRenderer(GLFWwindow *window);
    ~VulkanRenderer();
    void run();

private:
    void initVulkan();
    void initScene();
    void mainLoop();
    void cleanup();

    GLFWwindow *window;

    // Core Vulkan components
    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain;
    std::unique_ptr<VulkanRenderPass> vulkanRenderPass;
    std::unique_ptr<VulkanCommand> vulkanCommand;
    std::unique_ptr<VulkanSync> vulkanSync;
    std::unique_ptr<VulkanSurface> vulkanSurface;
    std::unique_ptr<VulkanFrame> vulkanFrame;

    // Rendering and scene
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<SceneBuilder> sceneBuilder;

    // Frame timing
    std::chrono::steady_clock::time_point lastFrameTime;
    uint32_t currentFrame = 0;
    const uint32_t MAX_FRAMES_IN_FLIGHT = 3;
};
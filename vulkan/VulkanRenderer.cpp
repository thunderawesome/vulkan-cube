#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"
#include "../src/Scene.h"
#include "../src/SceneBuilder.h"
#include "../src/Renderer.h"
#include "../src/FreeCamera.h"

#include <iostream>
#include <cstdlib>
#include <string>

VulkanRenderer::VulkanRenderer(GLFWwindow *window) : window(window)
{
    initVulkan();
    initScene();
}

VulkanRenderer::~VulkanRenderer()
{
    cleanup();
}

void VulkanRenderer::run()
{
    mainLoop();
}

void VulkanRenderer::initVulkan()
{
    // Initialize core Vulkan components
    vulkanInstance = std::make_unique<VulkanInstance>(true);
    vulkanSurface = std::make_unique<VulkanSurface>(*vulkanInstance, window);
    vulkanDevice = std::make_unique<VulkanDevice>(vulkanInstance->get(), vulkanSurface->get());
    vulkanSwapchain = std::make_unique<VulkanSwapchain>(*vulkanDevice, vulkanSurface->get(), window);
    vulkanRenderPass = std::make_unique<VulkanRenderPass>(*vulkanDevice, *vulkanSwapchain);
    vulkanSwapchain->createFramebuffers(vulkanRenderPass->get());

    vulkanCommand = std::make_unique<VulkanCommand>(*vulkanDevice, MAX_FRAMES_IN_FLIGHT);
    vulkanSync = std::make_unique<VulkanSync>(
        *vulkanDevice,
        static_cast<uint32_t>(vulkanSwapchain->getFramebuffers().size()),
        MAX_FRAMES_IN_FLIGHT);

    // Initialize rendering system
    renderer = std::make_unique<Renderer>();

    vulkanFrame = std::make_unique<VulkanFrame>(
        *vulkanDevice,
        *vulkanSwapchain,
        *vulkanRenderPass,
        *vulkanCommand,
        *vulkanSync,
        *renderer,
        MAX_FRAMES_IN_FLIGHT);

    lastFrameTime = std::chrono::steady_clock::now();
}

void VulkanRenderer::initScene()
{
    scene = std::make_unique<Scene>();
    sceneBuilder = std::make_unique<SceneBuilder>(*vulkanDevice, *vulkanRenderPass);

    sceneBuilder->createDemoScene(*scene);

    // === Free-Fly Camera ===
    auto camera = std::make_unique<GameObject>(nullptr, nullptr); // No mesh or material
    camera->transform.position = glm::vec3(0.0f, 2.0f, 8.0f);     // Good starting view
    camera->updateFunc = Behaviors::freeCamera(window, 6.0f, 0.15f);

    scene->addGameObject(std::move(camera));
}

void VulkanRenderer::handleInput()
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void VulkanRenderer::mainLoop()
{
    // Determine if we are in stress mode
    const char *stressEnv = std::getenv("STRESS_FRAMES");
    uint64_t targetFrames = 0;
    bool isStressTest = false;

    if (stressEnv)
    {
        try
        {
            targetFrames = std::stoull(std::string(stressEnv));
            isStressTest = true;
        }
        catch (...)
        {
        }
    }

    uint64_t frameCount = 0;

    while (!glfwWindowShouldClose(window))
    {
        // Exit if we've hit the stress test limit
        if (isStressTest && frameCount >= targetFrames)
            break;

        glfwPollEvents();
        handleInput(); // Centralized ESC check

        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        // Update scene logic (Physics, Camera, etc.)
        scene->update(deltaTime);

        // Render the frame
        auto result = vulkanFrame->draw(currentFrame, *scene);

        if (result == FrameResult::SwapchainOutOfDate)
        {
            vulkanSwapchain->recreate(vulkanRenderPass->get());
            vulkanFrame->updateTargetAspect();
        }
        else
        {
            // IMPORTANT: Increment frame index for synchronization primitives
            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        frameCount++;
    }

    vulkanDevice->getLogicalDevice().waitIdle();
}

void VulkanRenderer::cleanup()
{
    if (!vulkanDevice)
        return;

    // Wait for all GPU operations to finish before touching any resources
    vulkanDevice->getLogicalDevice().waitIdle();

    // 1. Destroy Scene and Builder
    // SceneBuilder likely owns Pipelines, PipelineLayouts, and DescriptorSetLayouts.
    // These MUST be destroyed while the Device still exists.
    scene.reset();
    sceneBuilder.reset();

    // 2. Destroy the Renderer
    // This may hold onto global resources like samplers or descriptor pools.
    renderer.reset();

    // 3. Destroy Frame-specific resources
    // vulkanFrame often contains per-frame command buffers and descriptors.
    vulkanFrame.reset();
    vulkanSync.reset();
    vulkanCommand.reset();

    // 4. Destroy Render Pass
    // Framebuffers (owned by Swapchain) depend on this, but resetting it here is safe
    // as long as the Framebuffers are cleared with the Swapchain below.
    vulkanRenderPass.reset();

    // 5. Destroy Swapchain
    // This destroys the ImageViews and Framebuffers.
    vulkanSwapchain.reset();

    // 6. Destroy Surface
    // Surface must be destroyed before the Instance, and after the Swapchain.
    vulkanSurface.reset();

    // 7. Core Device and Instance
    // The Device is the parent of almost everything above.
    // It must be the second-to-last thing to go.
    vulkanDevice.reset();
    vulkanInstance.reset();
}
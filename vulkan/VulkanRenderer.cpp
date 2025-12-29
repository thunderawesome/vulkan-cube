#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"
#include "src/Scene.h"
#include "src/SceneBuilder.h"
#include "src/Renderer.h"
#include "src/FreeCamera.h"

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

void VulkanRenderer::mainLoop()
{
    const char *stressEnv = std::getenv("STRESS_FRAMES");
    if (stressEnv)
    {
        uint64_t targetFrames = 0;
        try
        {
            targetFrames = std::stoull(std::string(stressEnv));
        }
        catch (...)
        {
            targetFrames = 0;
        }

        uint64_t frames = 0;
        while (frames < targetFrames && !glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // === ESC to exit early (even in stress mode) ===
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Release mouse
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
            }

            // Calculate delta time
            auto currentTime = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
            lastFrameTime = currentTime;

            // Update scene
            scene->update(deltaTime);

            // Draw frame
            auto result = vulkanFrame->draw(currentFrame, *scene);
            if (result == FrameResult::SwapchainOutOfDate)
            {
                vulkanSwapchain->recreate(vulkanRenderPass->get());
                vulkanFrame->updateTargetAspect();
            }

            ++frames;
        }
    }
    else
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            // === ESC to exit gracefully ===
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Release mouse
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            // Calculate delta time
            auto currentTime = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
            lastFrameTime = currentTime;

            // Update scene
            scene->update(deltaTime);

            // Draw frame
            auto result = vulkanFrame->draw(currentFrame, *scene);
            if (result == FrameResult::SwapchainOutOfDate)
            {
                vulkanSwapchain->recreate(vulkanRenderPass->get());
                vulkanFrame->updateTargetAspect();
            }
        }
    }

    vulkanDevice->getLogicalDevice().waitIdle();
}

void VulkanRenderer::cleanup()
{
    if (!vulkanDevice)
        return;

    vulkanDevice->getLogicalDevice().waitIdle();

    // Clean up in reverse order of dependencies
    scene.reset();
    sceneBuilder.reset(); // This owns meshes and materials
    renderer.reset();
    vulkanFrame.reset();
    vulkanSync.reset();
    vulkanCommand.reset();
    vulkanRenderPass.reset();
    vulkanSwapchain.reset();
    vulkanSurface.reset();
    vulkanDevice.reset();
    vulkanInstance.reset();
}
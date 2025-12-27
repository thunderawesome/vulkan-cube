#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"
#include "src/Mesh.h"
#include "src/Primitive.h"
#include "src/GameObject.h"

#include <iostream>
#include <cstdlib>
#include <string>

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

void VulkanRenderer::initVulkan()
{
    vulkanInstance = std::make_unique<VulkanInstance>(true);
    vulkanSurface = std::make_unique<VulkanSurface>(*vulkanInstance, window);
    vulkanDevice = std::make_unique<VulkanDevice>(vulkanInstance->get(), vulkanSurface->get());
    vulkanSwapchain = std::make_unique<VulkanSwapchain>(*vulkanDevice, vulkanSurface->get(), window);
    vulkanRenderPass = std::make_unique<VulkanRenderPass>(*vulkanDevice, *vulkanSwapchain);
    vulkanSwapchain->createFramebuffers(vulkanRenderPass->get());

    // Always use cube shaders for now (supports vertex buffers with transforms)
    std::string vertPath = "shaders/cube.vert.spv";
    std::string fragPath = "shaders/cube.frag.spv";

    auto shader = std::make_unique<VulkanShader>(*vulkanDevice, vertPath, fragPath);

    // Set up vertex input
    auto bindingDesc = Vertex::binding();
    auto attrs = Vertex::attributes();
    vulkanGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(
        *vulkanDevice, *vulkanRenderPass, *shader,
        &bindingDesc, static_cast<uint32_t>(attrs.size()), attrs.data());

    vulkanCommand = std::make_unique<VulkanCommand>(*vulkanDevice, MAX_FRAMES_IN_FLIGHT);
    vulkanSync = std::make_unique<VulkanSync>(
        *vulkanDevice,
        vulkanSwapchain->getFramebuffers().size(),
        MAX_FRAMES_IN_FLIGHT);

    vulkanFrame = std::make_unique<VulkanFrame>(
        *vulkanDevice,
        *vulkanSwapchain,
        *vulkanRenderPass,
        *vulkanGraphicsPipeline,
        *vulkanCommand,
        *vulkanSync,
        MAX_FRAMES_IN_FLIGHT);

    // Create meshes
    auto cubeVerts = Primitives::createCube();
    meshes.push_back(std::make_unique<Mesh>(*vulkanDevice, cubeVerts));

    auto triangleVerts = Primitives::createTriangle();
    meshes.push_back(std::make_unique<Mesh>(*vulkanDevice, triangleVerts));

    // Create game objects with different positions
    // Cube 1 - center with rotation
    Transform t1;
    t1.position = glm::vec3(0.0f, 0.0f, 0.0f);
    t1.rotation = glm::vec3(-25.0f, 45.0f, 0.0f);
    t1.scale = glm::vec3(1.0f);
    gameObjects.push_back(std::make_unique<GameObject>(meshes[0].get(), t1));

    // Cube 2 - to the right
    Transform t2;
    t2.position = glm::vec3(2.0f, 0.0f, 0.0f);
    t2.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    t2.scale = glm::vec3(0.5f); // smaller
    gameObjects.push_back(std::make_unique<GameObject>(meshes[0].get(), t2));

    // Cube 3 - to the left
    Transform t3;
    t3.position = glm::vec3(-2.0f, 0.0f, 0.0f);
    t3.rotation = glm::vec3(0.0f, 90.0f, 0.0f);
    t3.scale = glm::vec3(0.75f);
    gameObjects.push_back(std::make_unique<GameObject>(meshes[0].get(), t3));

    // Triangle - above center
    Transform t4;
    t4.position = glm::vec3(0.0f, 1.5f, 0.0f);
    t4.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    t4.scale = glm::vec3(1.5f);
    gameObjects.push_back(std::make_unique<GameObject>(meshes[1].get(), t4));

    // Register all game objects with the frame renderer
    for (auto &obj : gameObjects)
    {
        vulkanFrame->addGameObject(obj.get());
    }
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

            auto result = vulkanFrame->draw(currentFrame);

            if (result == FrameResult::SwapchainOutOfDate)
            {
                vulkanSwapchain->recreate(vulkanRenderPass->get());
                vulkanFrame->updateTargetAspect(); // <-- ADD THIS LINE
            }

            ++frames;
        }
    }
    else
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

            auto result = vulkanFrame->draw(currentFrame);

            if (result == FrameResult::SwapchainOutOfDate)
            {
                vulkanSwapchain->recreate(vulkanRenderPass->get());
                vulkanFrame->updateTargetAspect(); // <-- ADD THIS LINE
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

    // Clean up in reverse order
    vulkanFrame.reset();
    gameObjects.clear();
    meshes.clear();
    vulkanSync.reset();
    vulkanCommand.reset();
    vulkanGraphicsPipeline.reset();
    vulkanRenderPass.reset();
    vulkanSwapchain.reset();
    vulkanSurface.reset();
    vulkanDevice.reset();
    vulkanInstance.reset();
}
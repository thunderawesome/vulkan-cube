#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"
#include "Mesh.h"

#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <cstdlib>
#include <string>

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

    // Default to rendering the cube. Set `RENDER_TRIANGLE=1` to render the triangle instead.
    const char *renderTriangle = std::getenv("RENDER_TRIANGLE");
    std::string vertPath = "shaders/cube.vert.spv";
    std::string fragPath = "shaders/cube.frag.spv";
    uint32_t vc = 36;
    if (renderTriangle && std::string(renderTriangle) == "1")
    {
        vertPath = "shaders/triangle.vert.spv";
        fragPath = "shaders/triangle.frag.spv";
        vc = 3;
    }

    auto shader = std::make_unique<VulkanShader>(*vulkanDevice, vertPath, fragPath);

    // Select pipeline vertex input based on whether we're rendering the cube (uses vertex buffer)
    if (vc == 36)
    {
        auto bindingDesc = Mesh::Vertex::binding();
        auto attrs = Mesh::Vertex::attributes();
        vulkanGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*vulkanDevice, *vulkanRenderPass, *shader, &bindingDesc, static_cast<uint32_t>(attrs.size()), attrs.data());
    }
    else
    {
        vulkanGraphicsPipeline = std::make_unique<VulkanGraphicsPipeline>(*vulkanDevice, *vulkanRenderPass, *shader);
    }

    // create mesh (cube or triangle)
    std::vector<Mesh::Vertex> verts;
    if (vc == 36)
        verts = Mesh::createCubeVertices();
    else
    {
        // minimal triangle matching existing shader (using vertex index fallback if shader uses gl_VertexIndex)
        verts = {{{0.0f, -0.5f, 0.0f}, {1, 0, 0}}, {{0.5f, 0.5f, 0.0f}, {0, 1, 0}}, {{-0.5f, 0.5f, 0.0f}, {0, 0, 1}}};
    }

    vulkanCommand = std::make_unique<VulkanCommand>(*vulkanDevice, MAX_FRAMES_IN_FLIGHT);
    // mesh must be created after device is ready
    vulkanMesh = std::make_unique<Mesh>(*vulkanDevice, verts);
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
        *vulkanMesh,
        MAX_FRAMES_IN_FLIGHT);
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

    // All resources are cleaned up automatically by RAII destructors
    vulkanFrame.reset();
    vulkanSync.reset();
    vulkanCommand.reset();
    vulkanGraphicsPipeline.reset();
    vulkanRenderPass.reset();
    vulkanSwapchain.reset();
    vulkanSurface.reset();
    vulkanMesh.reset();
    vulkanDevice.reset();
    vulkanInstance.reset();
}
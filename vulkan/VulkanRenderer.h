#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>

#include "VulkanInstance.h"
#include "VulkanDevice.h"

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

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
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void drawFrame();

    void cleanupSwapChain();
    void recreateSwapChain();

    vk::ShaderModule vertShaderModule;
    vk::ShaderModule fragShaderModule;

    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);

    GLFWwindow *window;

    std::unique_ptr<VulkanInstance> instance;
    vk::SurfaceKHR surface;

    std::unique_ptr<VulkanDevice> device;

    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::ImageView> swapChainImageViews;
    std::vector<vk::Framebuffer> swapChainFramebuffers;

    vk::RenderPass renderPass;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;

    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;

    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;
    size_t currentFrame = 0;
    const int MAX_FRAMES_IN_FLIGHT = 2;

    const bool enableValidationLayers = true;
};
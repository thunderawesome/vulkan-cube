#pragma once
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>

class VulkanDevice; // Forward declaration

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class VulkanSwapchain
{
public:
    VulkanSwapchain(const VulkanDevice &device, vk::SurfaceKHR surface, GLFWwindow *window);
    ~VulkanSwapchain();

    vk::SwapchainKHR getSwapchain() const { return swapChain; }
    vk::Format getImageFormat() const { return swapChainImageFormat; }
    vk::Extent2D getExtent() const { return swapChainExtent; }
    const std::vector<vk::ImageView> &getImageViews() const { return swapChainImageViews; }
    const std::vector<vk::Framebuffer> &getFramebuffers() const { return swapChainFramebuffers; }
    vk::Framebuffer getFramebuffer(uint32_t index) const { return swapChainFramebuffers[index]; }

    void recreate(vk::RenderPass renderPass, GLFWwindow *window);
    void createFramebuffers(vk::RenderPass renderPass);

private:
    void createSwapchain();
    void createImageViews();

    void cleanupFramebuffers();
    void cleanup();

    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDevice);
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, GLFWwindow *window);

    const VulkanDevice &deviceRef; // Reference to VulkanDevice
    vk::SurfaceKHR surface;
    GLFWwindow *window;

    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::ImageView> swapChainImageViews;
    std::vector<vk::Framebuffer> swapChainFramebuffers;
};
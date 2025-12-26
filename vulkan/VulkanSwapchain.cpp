#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanSwapchain.h"
#include "VulkanDevice.h" // Needed for deviceRef getters

#include <algorithm>
#include <limits>

VulkanSwapchain::VulkanSwapchain(const VulkanDevice &device, vk::SurfaceKHR surface, GLFWwindow *window)
    : deviceRef(device), surface(surface), window(window)
{
    createSwapchain();
    createImageViews();
}

VulkanSwapchain::~VulkanSwapchain()
{
    cleanup();
}

void VulkanSwapchain::recreate(vk::RenderPass renderPass)
{
    // Wait for non-zero window size (minimized)
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    deviceRef.getLogicalDevice().waitIdle();

    cleanup();

    createSwapchain();
    createImageViews();
    createFramebuffers(renderPass);
}
void VulkanSwapchain::createSwapchain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(deviceRef.getPhysicalDevice());

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0)
    {
        imageCount = std::min(imageCount, swapChainSupport.capabilities.maxImageCount);
    }

    vk::SwapchainCreateInfoKHR createInfo(
        {},
        surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        swapChainSupport.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        true,
        {});

    uint32_t graphicsFamily = deviceRef.getGraphicsQueueFamily();
    uint32_t presentFamily = deviceRef.getPresentQueueFamily();

    if (graphicsFamily != presentFamily)
    {
        uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    swapChain = deviceRef.getLogicalDevice().createSwapchainKHR(createInfo);
    swapChainImages = deviceRef.getLogicalDevice().getSwapchainImagesKHR(swapChain);
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanSwapchain::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        vk::ImageViewCreateInfo createInfo(
            {},
            swapChainImages[i],
            vk::ImageViewType::e2D,
            swapChainImageFormat,
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        swapChainImageViews[i] = deviceRef.getLogicalDevice().createImageView(createInfo);
    }
}

void VulkanSwapchain::createFramebuffers(vk::RenderPass renderPass)
{
    cleanupFramebuffers(); // Optional: clean old ones if recreating

    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); ++i)
    {
        vk::FramebufferCreateInfo framebufferInfo(
            {},
            renderPass,
            1,
            &swapChainImageViews[i],
            swapChainExtent.width,
            swapChainExtent.height,
            1);

        swapChainFramebuffers[i] = deviceRef.getLogicalDevice().createFramebuffer(framebufferInfo);
    }
}

void VulkanSwapchain::cleanupFramebuffers()
{
    for (auto framebuffer : swapChainFramebuffers)
    {
        deviceRef.getLogicalDevice().destroyFramebuffer(framebuffer);
    }
    swapChainFramebuffers.clear();
}

void VulkanSwapchain::cleanup()
{
    cleanupFramebuffers();

    for (auto imageView : swapChainImageViews)
    {
        deviceRef.getLogicalDevice().destroyImageView(imageView);
    }
    swapChainImageViews.clear();

    if (swapChain)
    {
        deviceRef.getLogicalDevice().destroySwapchainKHR(swapChain);
        swapChain = nullptr;
    }

    swapChainImages.clear();
}

// ------------------------------------------------------------
// Swapchain Support Query Helpers
// ------------------------------------------------------------
SwapChainSupportDetails VulkanSwapchain::querySwapChainSupport(vk::PhysicalDevice physicalDevice)
{
    SwapChainSupportDetails details;
    details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    details.formats = physicalDevice.getSurfaceFormatsKHR(surface);
    details.presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
    return details;
}

vk::SurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
{
    for (const auto &format : availableFormats)
    {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
    for (const auto &mode : availablePresentModes)
    {
        if (mode == vk::PresentModeKHR::eMailbox)
        {
            return mode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanSwapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, GLFWwindow *window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}
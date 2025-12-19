#pragma once

#include <vulkan/vulkan.hpp>

class VulkanDevice;
class VulkanSwapchain;

class VulkanRenderPass
{
public:
    VulkanRenderPass(const VulkanDevice &device, const VulkanSwapchain &swapchain);
    ~VulkanRenderPass();

    vk::RenderPass get() const { return renderPass; }

private:
    vk::RenderPass renderPass;

    const VulkanDevice &deviceRef;
    const VulkanSwapchain &swapchainRef;
};
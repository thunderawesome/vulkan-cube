#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

class VulkanDevice;

class VulkanSync
{
public:
    VulkanSync(const VulkanDevice &device, uint32_t swapchainImageCount, uint32_t maxFramesInFlight);
    ~VulkanSync();

    vk::Semaphore getImageAvailableSemaphore(uint32_t frame) const { return imageAvailableSemaphores[frame]; }
    vk::Semaphore getRenderFinishedSemaphore(uint32_t imageIndex) const { return renderFinishedSemaphores[imageIndex]; }
    vk::Fence getInFlightFence(uint32_t frame) const { return inFlightFences[frame]; }

private:
    const VulkanDevice &deviceRef;

    std::vector<vk::Semaphore> imageAvailableSemaphores; // sized by maxFramesInFlight
    std::vector<vk::Semaphore> renderFinishedSemaphores; // sized by swapchain image count
    std::vector<vk::Fence> inFlightFences;               // sized by maxFramesInFlight
};
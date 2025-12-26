#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

class VulkanDevice;

class VulkanSync
{
public:
    VulkanSync(const VulkanDevice &device, uint32_t maxFramesInFlight);
    ~VulkanSync();

    vk::Semaphore getImageAvailableSemaphore(uint32_t frame) const { return imageAvailableSemaphores[frame]; }
    vk::Semaphore getRenderFinishedSemaphore(uint32_t frame) const { return renderFinishedSemaphores[frame]; }
    vk::Fence getInFlightFence(uint32_t frame) const { return inFlightFences[frame]; }

private:
    const VulkanDevice &deviceRef;

    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;
};
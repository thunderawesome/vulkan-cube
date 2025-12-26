#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

class VulkanDevice;

class VulkanCommand
{
public:
    VulkanCommand(const VulkanDevice &device, uint32_t maxFramesInFlight);
    ~VulkanCommand();

    vk::CommandBuffer getBuffer(uint32_t frameIndex) const { return commandBuffers[frameIndex]; }

private:
    void createCommandPool();
    void allocateCommandBuffers(uint32_t count);

    const VulkanDevice &deviceRef;
    vk::CommandPool commandPool;
    std::vector<vk::CommandBuffer> commandBuffers;
};
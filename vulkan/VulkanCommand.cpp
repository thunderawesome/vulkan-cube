#include "VulkanCommand.h"
#include "VulkanDevice.h"

VulkanCommand::VulkanCommand(const VulkanDevice &device, uint32_t maxFramesInFlight)
    : deviceRef(device)
{
    createCommandPool();
    allocateCommandBuffers(maxFramesInFlight);
}

VulkanCommand::~VulkanCommand()
{
    if (!commandBuffers.empty())
    {
        deviceRef.getLogicalDevice().freeCommandBuffers(commandPool, commandBuffers);
    }
    if (commandPool)
    {
        deviceRef.getLogicalDevice().destroyCommandPool(commandPool);
    }
}

void VulkanCommand::createCommandPool()
{
    uint32_t graphicsFamily = deviceRef.getGraphicsQueueFamily();

    vk::CommandPoolCreateInfo poolInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // Allows individual buffer reset
        graphicsFamily);

    commandPool = deviceRef.getLogicalDevice().createCommandPool(poolInfo);
}

void VulkanCommand::allocateCommandBuffers(uint32_t count)
{
    vk::CommandBufferAllocateInfo allocInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        count);

    commandBuffers = deviceRef.getLogicalDevice().allocateCommandBuffers(allocInfo);
}
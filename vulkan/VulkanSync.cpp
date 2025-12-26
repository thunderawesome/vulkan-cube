#include "VulkanSync.h"
#include "VulkanDevice.h"

VulkanSync::VulkanSync(const VulkanDevice &device, uint32_t maxFramesInFlight)
    : deviceRef(device)
{
    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(maxFramesInFlight);
    inFlightFences.resize(maxFramesInFlight);

    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

    for (uint32_t i = 0; i < maxFramesInFlight; ++i)
    {
        imageAvailableSemaphores[i] = deviceRef.getLogicalDevice().createSemaphore(semaphoreInfo);
        renderFinishedSemaphores[i] = deviceRef.getLogicalDevice().createSemaphore(semaphoreInfo);
        inFlightFences[i] = deviceRef.getLogicalDevice().createFence(fenceInfo);
    }
}

VulkanSync::~VulkanSync()
{
    for (auto semaphore : imageAvailableSemaphores)
    {
        deviceRef.getLogicalDevice().destroySemaphore(semaphore);
    }
    for (auto semaphore : renderFinishedSemaphores)
    {
        deviceRef.getLogicalDevice().destroySemaphore(semaphore);
    }
    for (auto fence : inFlightFences)
    {
        deviceRef.getLogicalDevice().destroyFence(fence);
    }
}
#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>
#include <vector>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanDevice
{
public:
    VulkanDevice(vk::Instance instance, vk::SurfaceKHR surface);
    ~VulkanDevice();

    vk::PhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    vk::Device getLogicalDevice() const { return device; }
    vk::Queue getGraphicsQueue() const { return graphicsQueue; }
    vk::Queue getPresentQueue() const { return presentQueue; }
    const QueueFamilyIndices &getQueueIndices() const { return queueIndices; }
    uint32_t getGraphicsQueueFamily() const { return queueIndices.graphicsFamily.value(); }
    uint32_t getPresentQueueFamily() const { return queueIndices.presentFamily.value(); }

private:
    void pickPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface);
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);

    vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;
    vk::Device device;

    QueueFamilyIndices queueIndices;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    const std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};
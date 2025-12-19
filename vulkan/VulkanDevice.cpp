#include "VulkanDevice.h"

#include <iostream>
#include <set>

VulkanDevice::VulkanDevice(vk::Instance instance, vk::SurfaceKHR surface)
{
    pickPhysicalDevice(instance, surface);

    std::set<uint32_t> uniqueQueueFamilies = {
        queueIndices.graphicsFamily.value(),
        queueIndices.presentFamily.value()};

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        queueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags(), queueFamily, 1, &queuePriority);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};

    vk::DeviceCreateInfo createInfo;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    device = physicalDevice.createDevice(createInfo);

    // VULKAN_HPP_DEFAULT_DISPATCHER.init(device); // Device-level function loading

    graphicsQueue = device.getQueue(queueIndices.graphicsFamily.value(), 0);
    presentQueue = device.getQueue(queueIndices.presentFamily.value(), 0);
}

VulkanDevice::~VulkanDevice()
{
    if (device)
    {
        device.destroy();
    }
}

void VulkanDevice::pickPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface)
{
    auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty())
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    for (const auto &dev : devices)
    {
        QueueFamilyIndices indices = findQueueFamilies(dev, surface);
        if (indices.isComplete())
        {
            physicalDevice = dev;
            queueIndices = indices; // Store only when we pick the device
            return;
        }
    }

    throw std::runtime_error("failed to find a suitable GPU!");
}

QueueFamilyIndices VulkanDevice::findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface)
{
    QueueFamilyIndices indices;

    auto queueFamilies = device.getQueueFamilyProperties();
    uint32_t i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }
        if (device.getSurfaceSupportKHR(i, surface))
        {
            indices.presentFamily = i;
        }
        if (indices.isComplete())
            break;
        ++i;
    }

    return indices;
}
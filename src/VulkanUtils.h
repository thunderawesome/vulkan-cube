#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <functional>
#include "../vulkan/VulkanDevice.h"

namespace VulkanUtils
{
    void copyToDeviceBuffer(const VulkanDevice &device,
                            vk::Buffer dstBuffer,
                            const void *data,
                            vk::DeviceSize size);

    template <typename T>
    inline void uploadBuffer(const VulkanDevice &device,
                             vk::BufferUsageFlags usage,
                             const std::vector<T> &data,
                             vk::Buffer &outBuffer,
                             vk::DeviceMemory &outMemory)
    {
        vk::DeviceSize bufferSize = sizeof(T) * data.size();

        vk::BufferCreateInfo bufferInfo;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usage | vk::BufferUsageFlagBits::eTransferDst;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        outBuffer = device.getLogicalDevice().createBuffer(bufferInfo);

        auto memReq = device.getLogicalDevice().getBufferMemoryRequirements(outBuffer);

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

        outMemory = device.getLogicalDevice().allocateMemory(allocInfo);
        device.getLogicalDevice().bindBufferMemory(outBuffer, outMemory, 0);

        copyToDeviceBuffer(device, outBuffer, data.data(), bufferSize);
    }

    void uploadTexture(const VulkanDevice &device,
                       const void *pixels,
                       uint32_t width,
                       uint32_t height,
                       vk::Image &outImage,
                       vk::DeviceMemory &outMemory);

    void transitionImageLayout(const VulkanDevice &device,
                               vk::Image image,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout);
}
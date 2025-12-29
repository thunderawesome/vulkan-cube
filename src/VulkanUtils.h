#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <functional>

class VulkanDevice;

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

        vk::BufferCreateInfo bufferInfo({}, bufferSize, usage | vk::BufferUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive);
        outBuffer = device.getLogicalDevice().createBuffer(bufferInfo);

        auto memReq = device.getLogicalDevice().getBufferMemoryRequirements(outBuffer);
        vk::MemoryAllocateInfo allocInfo(memReq.size, device.findMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
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
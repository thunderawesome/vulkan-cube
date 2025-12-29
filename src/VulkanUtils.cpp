#include "VulkanUtils.h"
#include "../vulkan/VulkanDevice.h"
#include <stdexcept>

namespace VulkanUtils
{
    void immediateSubmit(const VulkanDevice &device, std::function<void(vk::CommandBuffer)> recorder)
    {
        vk::CommandPoolCreateInfo poolInfo({}, device.getGraphicsQueueFamily());
        vk::CommandPool cmdPool = device.getLogicalDevice().createCommandPool(poolInfo);

        vk::CommandBufferAllocateInfo allocInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1);
        vk::CommandBuffer cmd = device.getLogicalDevice().allocateCommandBuffers(allocInfo)[0];

        vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        cmd.begin(beginInfo);
        recorder(cmd);
        cmd.end();

        vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &cmd);
        device.getGraphicsQueue().submit(submitInfo);
        device.getGraphicsQueue().waitIdle();

        device.getLogicalDevice().freeCommandBuffers(cmdPool, cmd);
        device.getLogicalDevice().destroyCommandPool(cmdPool);
    }

    void copyToDeviceBuffer(const VulkanDevice &device,
                            vk::Buffer dstBuffer,
                            const void *data,
                            vk::DeviceSize size)
    {
        vk::BufferCreateInfo stagingInfo({}, size, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
        vk::Buffer stagingBuffer = device.getLogicalDevice().createBuffer(stagingInfo);

        auto memReq = device.getLogicalDevice().getBufferMemoryRequirements(stagingBuffer);
        vk::MemoryAllocateInfo allocInfo(memReq.size,
                                         device.findMemoryType(memReq.memoryTypeBits,
                                                               vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
        vk::DeviceMemory stagingMemory = device.getLogicalDevice().allocateMemory(allocInfo);
        device.getLogicalDevice().bindBufferMemory(stagingBuffer, stagingMemory, 0);

        void *mapped = device.getLogicalDevice().mapMemory(stagingMemory, 0, size);
        memcpy(mapped, data, size);
        device.getLogicalDevice().unmapMemory(stagingMemory);

        immediateSubmit(device, [&](vk::CommandBuffer cmd)
                        {
            vk::BufferCopy copyRegion(0, 0, size);
            cmd.copyBuffer(stagingBuffer, dstBuffer, copyRegion); });

        device.getLogicalDevice().destroyBuffer(stagingBuffer);
        device.getLogicalDevice().freeMemory(stagingMemory);
    }

    void transitionImageLayout(const VulkanDevice &device,
                               vk::Image image,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout)
    {
        immediateSubmit(device, [&](vk::CommandBuffer cmd)
                        {
            vk::ImageMemoryBarrier barrier({});
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

            vk::PipelineStageFlags sourceStage;
            vk::PipelineStageFlags destinationStage;

            if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
            {
                barrier.srcAccessMask = {};
                barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
                sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
                destinationStage = vk::PipelineStageFlagBits::eTransfer;
            }
            else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
            {
                barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
                sourceStage = vk::PipelineStageFlagBits::eTransfer;
                destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
            }
            else
            {
                throw std::runtime_error("Unsupported layout transition!");
            }

            cmd.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier); });
    }

    void uploadTexture(const VulkanDevice &device,
                       const void *pixels,
                       uint32_t width,
                       uint32_t height,
                       vk::Image &outImage,
                       vk::DeviceMemory &outMemory)
    {
        vk::DeviceSize imageSize = width * height * 4;

        // Staging buffer
        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingMemory;
        std::vector<char> tempData((char *)pixels, (char *)pixels + imageSize);
        uploadBuffer(device, vk::BufferUsageFlagBits::eTransferSrc, tempData, stagingBuffer, stagingMemory);

        // Create image
        vk::ImageCreateInfo imageInfo({}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Srgb,
                                      vk::Extent3D(width, height, 1), 1, 1, vk::SampleCountFlagBits::e1,
                                      vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
        outImage = device.getLogicalDevice().createImage(imageInfo);

        auto memReq = device.getLogicalDevice().getImageMemoryRequirements(outImage);
        vk::MemoryAllocateInfo allocInfo(memReq.size, device.findMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
        outMemory = device.getLogicalDevice().allocateMemory(allocInfo);
        device.getLogicalDevice().bindImageMemory(outImage, outMemory, 0);

        transitionImageLayout(device, outImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        immediateSubmit(device, [&](vk::CommandBuffer cmd)
                        {
            vk::BufferImageCopy region({}, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                       vk::Offset3D(0, 0, 0), vk::Extent3D(width, height, 1));
            cmd.copyBufferToImage(stagingBuffer, outImage, vk::ImageLayout::eTransferDstOptimal, region); });

        transitionImageLayout(device, outImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        device.getLogicalDevice().destroyBuffer(stagingBuffer);
        device.getLogicalDevice().freeMemory(stagingMemory);
    }
}
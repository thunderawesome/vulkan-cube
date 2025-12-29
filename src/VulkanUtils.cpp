#include "VulkanUtils.h"
#include <stdexcept>

namespace VulkanUtils
{
    void immediateSubmit(const VulkanDevice &device, std::function<void(vk::CommandBuffer)> recorder)
    {
        vk::CommandPoolCreateInfo poolInfo;
        poolInfo.queueFamilyIndex = device.getGraphicsQueueFamily();

        vk::CommandPool cmdPool = device.getLogicalDevice().createCommandPool(poolInfo);

        vk::CommandBufferAllocateInfo allocInfo;
        allocInfo.commandPool = cmdPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = 1;

        vk::CommandBuffer cmd = device.getLogicalDevice().allocateCommandBuffers(allocInfo)[0];

        vk::CommandBufferBeginInfo beginInfo;
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        cmd.begin(beginInfo);
        recorder(cmd);
        cmd.end();

        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

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
        vk::BufferCreateInfo stagingInfo;
        stagingInfo.size = size;
        stagingInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
        stagingInfo.sharingMode = vk::SharingMode::eExclusive;

        vk::Buffer stagingBuffer = device.getLogicalDevice().createBuffer(stagingInfo);

        auto memReq = device.getLogicalDevice().getBufferMemoryRequirements(stagingBuffer);

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(memReq.memoryTypeBits,
                                                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        vk::DeviceMemory stagingMemory = device.getLogicalDevice().allocateMemory(allocInfo);
        device.getLogicalDevice().bindBufferMemory(stagingBuffer, stagingMemory, 0);

        void *mapped = device.getLogicalDevice().mapMemory(stagingMemory, 0, size);
        memcpy(mapped, data, size);
        device.getLogicalDevice().unmapMemory(stagingMemory);

        immediateSubmit(device, [&](vk::CommandBuffer cmd)
                        {
            vk::BufferCopy copyRegion;
            copyRegion.size = size;
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
            vk::ImageMemoryBarrier barrier;
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
                barrier.srcAccessMask = vk::AccessFlags();
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

            cmd.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), {}, {}, barrier); });
    }

    void uploadTexture(const VulkanDevice &device,
                       const void *pixels,
                       uint32_t width,
                       uint32_t height,
                       vk::Image &outImage,
                       vk::DeviceMemory &outMemory)
    {
        vk::DeviceSize imageSize = width * height * 4;

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingMemory;

        // Note: Using explicit vector initialization here
        std::vector<char> tempData((const char *)pixels, (const char *)pixels + imageSize);

        // Call the template from the header
        uploadBuffer(device, vk::BufferUsageFlagBits::eTransferSrc, tempData, stagingBuffer, stagingMemory);

        vk::ImageCreateInfo imageInfo;
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = vk::Format::eR8G8B8A8Srgb;
        imageInfo.extent = vk::Extent3D(width, height, 1);
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;

        outImage = device.getLogicalDevice().createImage(imageInfo);

        auto memReq = device.getLogicalDevice().getImageMemoryRequirements(outImage);

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

        outMemory = device.getLogicalDevice().allocateMemory(allocInfo);
        device.getLogicalDevice().bindImageMemory(outImage, outMemory, 0);

        transitionImageLayout(device, outImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        immediateSubmit(device, [&](vk::CommandBuffer cmd)
                        {
            vk::BufferImageCopy region;
            region.imageSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
            region.imageExtent = vk::Extent3D(width, height, 1);
            cmd.copyBufferToImage(stagingBuffer, outImage, vk::ImageLayout::eTransferDstOptimal, region); });

        transitionImageLayout(device, outImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        device.getLogicalDevice().destroyBuffer(stagingBuffer);
        device.getLogicalDevice().freeMemory(stagingMemory);
    }
}
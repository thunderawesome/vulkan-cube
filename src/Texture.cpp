#include "Texture.h"
#include "../vulkan/VulkanDevice.h"
#include <stb_image.h>
#include <stdexcept>

Texture::Texture(const VulkanDevice &device, const std::string &filename)
    : deviceRef(device)
{
    createImage(filename);
    createImageView();
    createSampler();
}

Texture::~Texture()
{
    deviceRef.getLogicalDevice().destroySampler(sampler);
    deviceRef.getLogicalDevice().destroyImageView(imageView);
    deviceRef.getLogicalDevice().destroyImage(image);
    deviceRef.getLogicalDevice().freeMemory(imageMemory);
}

void Texture::createImage(const std::string &filename)
{
    int texWidth, texHeight, texChannels;

    // NEW: Flip the image vertically on load to match Vulkan's UV coordinate system (0,0 at top-left)
    stbi_set_flip_vertically_on_load(true);

    stbi_uc *pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image: " + filename);
    }

    vk::DeviceSize imageSize = texWidth * texHeight * 4;
    auto device = deviceRef.getLogicalDevice();

    // === Create staging buffer ===
    vk::BufferCreateInfo stagingBufferInfo({}, imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
    vk::Buffer stagingBuffer = device.createBuffer(stagingBufferInfo);

    vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(stagingBuffer);
    vk::MemoryAllocateInfo allocInfo(memRequirements.size,
                                     deviceRef.findMemoryType(memRequirements.memoryTypeBits,
                                                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
    vk::DeviceMemory stagingMemory = device.allocateMemory(allocInfo);
    device.bindBufferMemory(stagingBuffer, stagingMemory, 0);

    void *data = device.mapMemory(stagingMemory, 0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    device.unmapMemory(stagingMemory);

    stbi_image_free(pixels);

    // === Create optimal tiled image ===
    vk::ImageCreateInfo imageInfo({}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Srgb,
                                  vk::Extent3D(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1),
                                  1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                                  vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                                  vk::SharingMode::eExclusive);
    image = device.createImage(imageInfo);

    memRequirements = device.getImageMemoryRequirements(image);
    allocInfo = vk::MemoryAllocateInfo(memRequirements.size,
                                       deviceRef.findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    imageMemory = device.allocateMemory(allocInfo);
    device.bindImageMemory(image, imageMemory, 0);

    // === One-time command buffer for layout transition and copy ===
    vk::CommandPoolCreateInfo poolInfo({}, deviceRef.getGraphicsQueueFamily());
    vk::CommandPool commandPool = device.createCommandPool(poolInfo);

    vk::CommandBufferAllocateInfo cmdAllocInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(cmdAllocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer.begin(beginInfo);

    // Transition to TRANSFER_DST
    vk::ImageMemoryBarrier barrier({});
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.srcAccessMask = {};
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

    // Copy staging buffer to image
    vk::BufferImageCopy copyRegion({}, 0, 0,
                                   vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                   vk::Offset3D(0, 0, 0),
                                   vk::Extent3D(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1));
    commandBuffer.copyBufferToImage(stagingBuffer, image, vk::ImageLayout::eTransferDstOptimal, copyRegion);

    // Transition to SHADER_READ_ONLY
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

    commandBuffer.end();

    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer);
    deviceRef.getGraphicsQueue().submit(submitInfo);
    deviceRef.getGraphicsQueue().waitIdle();

    // Cleanup
    device.freeCommandBuffers(commandPool, commandBuffer);
    device.destroyCommandPool(commandPool);
    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingMemory);
}

void Texture::createImageView()
{
    vk::ImageViewCreateInfo viewInfo({}, image, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Srgb,
                                     {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
    imageView = deviceRef.getLogicalDevice().createImageView(viewInfo);
}

void Texture::createSampler()
{
    vk::SamplerCreateInfo samplerInfo({});
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f; // No mipmaps
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;

    sampler = deviceRef.getLogicalDevice().createSampler(samplerInfo);
}
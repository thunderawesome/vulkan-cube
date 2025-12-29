#pragma once
#include <vulkan/vulkan.hpp>
#include <string>

class VulkanDevice;

class Texture
{
public:
    Texture(const VulkanDevice &device, const std::string &filename);
    ~Texture();

    vk::ImageView getImageView() const { return imageView; }
    vk::Sampler getSampler() const { return sampler; }

private:
    const VulkanDevice &deviceRef;
    vk::Image image;
    vk::DeviceMemory imageMemory;
    vk::ImageView imageView;
    vk::Sampler sampler;

    void createImage(const std::string &filename);
    void createImageView();
    void createSampler();
};
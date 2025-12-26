#pragma once

#include <vulkan/vulkan.hpp>
#include <string>

class VulkanDevice;

class VulkanShader
{
public:
    // Load from SPIR-V file paths
    VulkanShader(const VulkanDevice &device,
                 const std::string &vertPath,
                 const std::string &fragPath);

    // Reserved for future use (e.g., embedded SPIR-V)
    VulkanShader(const VulkanDevice &device);

    ~VulkanShader();

    vk::ShaderModule getVertexModule() const { return vertexModule; }
    vk::ShaderModule getFragmentModule() const { return fragmentModule; }

private:
    vk::ShaderModule loadModule(const std::string &path);

    const VulkanDevice &deviceRef;
    vk::ShaderModule vertexModule = nullptr;
    vk::ShaderModule fragmentModule = nullptr;
};
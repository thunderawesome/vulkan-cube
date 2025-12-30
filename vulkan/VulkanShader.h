#pragma once

#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>

class VulkanDevice;

class VulkanShader
{
public:
    // Legacy support: Paths are now ignored in favor of embedded assets
    VulkanShader(const VulkanDevice &device,
                 const std::string &vertPath,
                 const std::string &fragPath);

    // Primary constructor: Loads embedded SPIR-V shaders automatically
    VulkanShader(const VulkanDevice &device);

    ~VulkanShader();

    vk::ShaderModule getVertexModule() const { return vertexModule; }
    vk::ShaderModule getFragmentModule() const { return fragmentModule; }

private:
    // Helper to create module from embedded byte arrays
    vk::ShaderModule createShaderModule(const std::vector<uint32_t> &code);

    // Kept for signature compatibility if needed
    vk::ShaderModule loadModule(const std::string &path);

    const VulkanDevice &deviceRef;
    vk::ShaderModule vertexModule = nullptr;
    vk::ShaderModule fragmentModule = nullptr;
};
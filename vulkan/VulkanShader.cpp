#include "VulkanShader.h"
#include "VulkanDevice.h"

#include <fstream>
#include <vector>
#include <stdexcept>

namespace
{

    // Private helper — only used in this file
    std::vector<char> readFile(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open shader file: " + filename);
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

}

VulkanShader::VulkanShader(const VulkanDevice &device,
                           const std::string &vertPath,
                           const std::string &fragPath)
    : deviceRef(device)
{
    vertexModule = loadModule(vertPath);
    fragmentModule = loadModule(fragPath);
}

VulkanShader::VulkanShader(const VulkanDevice &device)
    : deviceRef(device)
{
    // Reserved for future use (e.g., embedded SPIR-V or default shaders)
}

VulkanShader::~VulkanShader()
{
    if (vertexModule)
    {
        deviceRef.getLogicalDevice().destroyShaderModule(vertexModule);
    }
    if (fragmentModule)
    {
        deviceRef.getLogicalDevice().destroyShaderModule(fragmentModule);
    }
}

vk::ShaderModule VulkanShader::loadModule(const std::string &path)
{
    auto code = readFile(path);

    vk::ShaderModuleCreateInfo createInfo(
        {},
        code.size(),
        reinterpret_cast<const uint32_t *>(code.data()));

    return deviceRef.getLogicalDevice().createShaderModule(createInfo);
}
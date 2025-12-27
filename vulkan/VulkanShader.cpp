#include "VulkanShader.h"
#include "VulkanDevice.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif

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
    std::vector<char> code;

    // Try given path first, then fall back to executable directory and its parent (common CMake build layout)
    try
    {
        code = readFile(path);
        std::cerr << "Loaded shader SPIR-V from: " << path << std::endl;
    }
    catch (...)
    {
        std::filesystem::path p(path);

        // determine exe directory
        std::filesystem::path exeDir;
#ifdef _WIN32
        char buf[MAX_PATH];
        if (GetModuleFileNameA(NULL, buf, MAX_PATH) != 0)
        {
            exeDir = std::filesystem::path(buf).parent_path();
        }
        else
        {
            exeDir = std::filesystem::current_path();
        }
#else
        exeDir = std::filesystem::current_path();
#endif

        std::vector<std::filesystem::path> candidates = {
            exeDir / p,
            exeDir.parent_path() / p,
            exeDir / "shaders" / p.filename(),
            std::filesystem::current_path() / p.filename()};

        bool loaded = false;
        for (const auto &cand : candidates)
        {
            try
            {
                code = readFile(cand.string());
                loaded = true;
                std::cerr << "Loaded shader SPIR-V from candidate: " << cand.string() << std::endl;
                break;
            }
            catch (...)
            {
            }
        }

        if (!loaded)
            throw std::runtime_error("failed to open shader file: " + path);
    }

    vk::ShaderModuleCreateInfo createInfo(
        {},
        code.size(),
        reinterpret_cast<const uint32_t *>(code.data()));

    return deviceRef.getLogicalDevice().createShaderModule(createInfo);
}
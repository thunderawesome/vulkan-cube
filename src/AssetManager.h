#pragma once
#include <unordered_map>
#include <string>
#include <memory>

class VulkanDevice;
class Texture;

class AssetManager
{
public:
    explicit AssetManager(const VulkanDevice &device);
    ~AssetManager();

    // Load texture if not already loaded, returns pointer (never nullptr)
    Texture *loadTexture(const std::string &filename);

private:
    const VulkanDevice &deviceRef;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
};
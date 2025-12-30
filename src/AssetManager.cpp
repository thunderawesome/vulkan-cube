#include "AssetManager.h"
#include "Texture.h"
#include <stdexcept>

AssetManager::AssetManager(const VulkanDevice &device)
    : deviceRef(device)
{
}

AssetManager::~AssetManager() = default;

Texture *AssetManager::loadTexture(const std::string &filename)
{
    auto it = textures.find(filename);
    if (it != textures.end())
    {
        return it->second.get();
    }

    // Note: Texture currently ignores 'filename' and uses embedded data
    auto texture = std::make_unique<Texture>(deviceRef, filename);
    Texture *ptr = texture.get();
    textures[filename] = std::move(texture);
    return ptr;
}
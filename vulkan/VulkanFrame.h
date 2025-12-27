#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

class VulkanDevice;
class VulkanSwapchain;
class VulkanRenderPass;
class VulkanCommand;
class VulkanSync;
class Mesh;
class Material;
struct GameObject;

enum class FrameResult
{
    Success,
    SwapchainOutOfDate
};

class VulkanFrame
{
public:
    VulkanFrame(const VulkanDevice &device,
                const VulkanSwapchain &swapchain,
                const VulkanRenderPass &renderPass,
                VulkanCommand &command,
                VulkanSync &sync,
                uint32_t maxFramesInFlight);

    FrameResult draw(uint32_t &currentFrame);

    // Add/remove game objects
    void addGameObject(GameObject *obj);
    void clearGameObjects();

    // Update target aspect ratio (call after swapchain recreation)
    void updateTargetAspect();

private:
    const VulkanDevice &deviceRef;
    const VulkanSwapchain &swapchainRef;
    const VulkanRenderPass &renderPassRef;
    VulkanCommand &commandRef;
    VulkanSync &syncRef;

    std::vector<GameObject *> gameObjects;

    const uint32_t maxFramesInFlight;
    float targetAspect = 1.0f;

    // Helper to batch objects by material for efficient rendering
    void renderObjects(vk::CommandBuffer cmd,
                       const glm::mat4 &view,
                       const glm::mat4 &proj);
};
#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

class VulkanDevice;
class VulkanSwapchain;
class VulkanRenderPass;
class VulkanCommand;
class VulkanSync;
class Renderer;
class Scene;

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
                Renderer &renderer,
                uint32_t maxFramesInFlight);

    FrameResult draw(uint32_t &currentFrame, const Scene &scene);
    void updateTargetAspect();

private:
    const VulkanDevice &deviceRef;
    const VulkanSwapchain &swapchainRef;
    const VulkanRenderPass &renderPassRef;
    VulkanCommand &commandRef;
    VulkanSync &syncRef;
    Renderer &rendererRef;

    const uint32_t maxFramesInFlight;
    float targetAspect = 1.0f;

    glm::mat4 viewMatrix = glm::mat4(1.0f);
    glm::mat4 projMatrix = glm::mat4(1.0f);

    void setupViewportAndScissor(vk::CommandBuffer cmd);
    glm::mat4 getCameraViewMatrix(const Scene &scene) const;
};
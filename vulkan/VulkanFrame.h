#pragma once

#include <vulkan/vulkan.hpp>

class VulkanDevice;
class VulkanSwapchain;
class VulkanRenderPass;
class VulkanGraphicsPipeline;
class VulkanCommand;
class VulkanSync;

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
                const VulkanGraphicsPipeline &pipeline,
                VulkanCommand &command,
                VulkanSync &sync,
                uint32_t maxFramesInFlight);

    FrameResult draw(uint32_t &currentFrame);

private:
    const VulkanDevice &deviceRef;
    const VulkanSwapchain &swapchainRef;
    const VulkanRenderPass &renderPassRef;
    const VulkanGraphicsPipeline &pipelineRef;
    VulkanCommand &commandRef;
    VulkanSync &syncRef;

    const uint32_t maxFramesInFlight;
};
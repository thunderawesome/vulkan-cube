#pragma once
#include <vulkan/vulkan.hpp>
#include <memory>

class VulkanDevice;
class VulkanRenderPass;
class VulkanShader;
class VulkanGraphicsPipeline;

class Material
{
public:
    Material(const VulkanDevice &device,
             const VulkanRenderPass &renderPass,
             std::unique_ptr<VulkanShader> shader);

    ~Material();

    // Delete copy, allow move
    Material(const Material &) = delete;
    Material &operator=(const Material &) = delete;
    Material(Material &&) noexcept;
    Material &operator=(Material &&) noexcept;

    vk::Pipeline getPipeline() const;
    vk::PipelineLayout getLayout() const;

private:
    std::unique_ptr<VulkanShader> shader;
    std::unique_ptr<VulkanGraphicsPipeline> pipeline;
};
#pragma once
#include <vulkan/vulkan.hpp>
#include <memory>

class VulkanDevice;
class VulkanRenderPass;
class VulkanShader;
class VulkanGraphicsPipeline;
class Texture;

class Material
{
public:
    // Change to const VulkanDevice& to match SceneBuilder's deviceRef
    Material(const VulkanDevice &device,
             const VulkanRenderPass &renderPass,
             std::unique_ptr<VulkanShader> shader,
             vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack);

    ~Material();

    // No copying Vulkan resources
    Material(const Material &) = delete;
    Material &operator=(const Material &) = delete;

    // Custom move operations because we have a destructor and unique handles
    Material(Material &&other) noexcept;
    Material &operator=(Material &&other) noexcept;

    vk::Pipeline getPipeline() const;
    vk::PipelineLayout getLayout() const;
    vk::DescriptorSet getDescriptorSet() const { return descriptorSet; }

    void setAlbedoTexture(Texture *texture);

private:
    const VulkanDevice *deviceRef; // Pointer allows for moving/reassignment

    std::unique_ptr<VulkanShader> shader;
    std::unique_ptr<VulkanGraphicsPipeline> pipeline;

    Texture *albedoTexture = nullptr;

    vk::DescriptorPool descriptorPool = nullptr;
    vk::DescriptorSet descriptorSet = nullptr;

    void createDescriptorResources();
    void updateDescriptorSet();
};
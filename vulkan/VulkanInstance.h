#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>

class VulkanInstance
{
public:
    VulkanInstance(bool enableValidationLayers = true);
    ~VulkanInstance();

    vk::Instance get() const { return instance; }

private:
    void createInstance();
    void setupDebugMessenger();

    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;

    bool enableValidationLayers;
    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};
};
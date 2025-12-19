#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "VulkanInstance.h"

#include <iostream>

// Static initialization of the default dispatcher (core functions)
static int initDispatcher = (VULKAN_HPP_DEFAULT_DISPATCHER.init(), 0);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

VulkanInstance::VulkanInstance(bool enableValidationLayers)
    : enableValidationLayers(enableValidationLayers)
{
    createInstance();
    if (enableValidationLayers)
    {
        setupDebugMessenger();
    }
}

VulkanInstance::~VulkanInstance()
{
    if (enableValidationLayers && debugMessenger)
    {
        instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }
    if (instance)
    {
        instance.destroy();
    }
}

void VulkanInstance::createInstance()
{
    vk::ApplicationInfo appInfo("Vulkan Cube", VK_MAKE_VERSION(1, 0, 0), "No Engine", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_3);

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    vk::InstanceCreateInfo createInfo({}, &appInfo, 0, nullptr, extensions.size(), extensions.data());

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    instance = vk::createInstance(createInfo);

    // Critical: Initialize dispatcher with the instance for extension functions
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

void VulkanInstance::setupDebugMessenger()
{
    vk::DebugUtilsMessengerCreateInfoEXT createInfo(
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        debugCallback);

    debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
}
#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

class VulkanInstance;

class VulkanSurface
{
public:
    VulkanSurface(const VulkanInstance &instance, GLFWwindow *window);
    ~VulkanSurface();

    vk::SurfaceKHR get() const { return surface; }

private:
    const VulkanInstance &instanceRef;
    vk::SurfaceKHR surface;
};
#include "VulkanSurface.h"
#include "VulkanInstance.h"

#include <GLFW/glfw3.h>

VulkanSurface::VulkanSurface(const VulkanInstance &instance, GLFWwindow *window)
    : instanceRef(instance)
{
    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    VkResult result = glfwCreateWindowSurface(
        static_cast<VkInstance>(instanceRef.get()),
        window,
        nullptr,
        &rawSurface);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }

    surface = vk::SurfaceKHR(rawSurface);
}

VulkanSurface::~VulkanSurface()
{
    if (surface)
    {
        instanceRef.get().destroySurfaceKHR(surface);
    }
}
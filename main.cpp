#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include "VulkanRenderer.h"

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan Window", nullptr, nullptr);

    try
    {
        VulkanRenderer renderer(window);
        renderer.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "CRASHED WITH EXCEPTION: " << e.what() << std::endl;
        std::cin.get(); // Pause to see the message
        return -1;
    }
    catch (...)
    {
        std::cerr << "CRASHED WITH UNKNOWN EXCEPTION" << std::endl;
        std::cin.get();
        return -1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
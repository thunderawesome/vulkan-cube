#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include "VulkanRenderer.h"

#include <iostream>
#include <set>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <vector>

// Static initialization of the default dispatcher (core functions)
static int initDispatcher = (VULKAN_HPP_DEFAULT_DISPATCHER.init(), 0);

// ------------------------------------------------------------
// Helper to load SPIR-V from file
// ------------------------------------------------------------
static std::vector<char> readFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file: " + filename);
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

// ------------------------------------------------------------
// Debug callback
// ------------------------------------------------------------
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

// ------------------------------------------------------------
// Constructor / Destructor / Run
// ------------------------------------------------------------
VulkanRenderer::VulkanRenderer(GLFWwindow *window) : window(window)
{
    initVulkan();
}

VulkanRenderer::~VulkanRenderer()
{
    cleanup();
}

void VulkanRenderer::run()
{
    mainLoop();
}

// ------------------------------------------------------------
// Init / Main Loop / Cleanup
// ------------------------------------------------------------
void VulkanRenderer::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void VulkanRenderer::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }

    device.waitIdle();
}

void VulkanRenderer::cleanup()
{
    for (auto &semaphore : imageAvailableSemaphores)
        device.destroySemaphore(semaphore);
    for (auto &semaphore : renderFinishedSemaphores)
        device.destroySemaphore(semaphore);
    for (auto &fence : inFlightFences)
        device.destroyFence(fence);

    device.freeCommandBuffers(commandPool, commandBuffers);
    device.destroyCommandPool(commandPool);

    for (auto framebuffer : swapChainFramebuffers)
        device.destroyFramebuffer(framebuffer);

    device.destroyPipeline(graphicsPipeline);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyRenderPass(renderPass);

    for (auto imageView : swapChainImageViews)
        device.destroyImageView(imageView);

    device.destroySwapchainKHR(swapChain);
    cleanupSwapChain();
    device.destroy();

    if (enableValidationLayers)
    {
        instance.destroyDebugUtilsMessengerEXT(debugMessenger, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
    }

    instance.destroySurfaceKHR(surface);
    instance.destroy();
}

// ------------------------------------------------------------
// Instance Creation
// ------------------------------------------------------------
void VulkanRenderer::createInstance()
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

    // Create instance first (no layers yet)
    instance = vk::createInstance(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    // Now check for validation layers (safe, dispatcher is initialized)
    if (enableValidationLayers)
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr); // Raw C function

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); // Raw C function

        for (const char *layerName : validationLayers)
        {
            bool found = false;
            for (const auto &properties : availableLayers)
            {
                if (strcmp(layerName, properties.layerName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                throw std::runtime_error("validation layer not available: " + std::string(layerName));
            }
        }
    }
}

// ------------------------------------------------------------
// Debug Messenger
// ------------------------------------------------------------
void VulkanRenderer::setupDebugMessenger()
{
    if (!enableValidationLayers)
        return;

    vk::DebugUtilsMessengerCreateInfoEXT createInfo(
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        debugCallback);

    debugMessenger = instance.createDebugUtilsMessengerEXT(createInfo, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
}

// ------------------------------------------------------------
// Surface Creation
// ------------------------------------------------------------
void VulkanRenderer::createSurface()
{
    VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
    VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(instance), window, nullptr, &rawSurface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::SurfaceKHR(rawSurface);
}

// ------------------------------------------------------------
// Physical Device Selection
// ------------------------------------------------------------
void VulkanRenderer::pickPhysicalDevice()
{
    auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty())
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    for (const auto &device : devices)
    {
        if (findQueueFamilies(device).isComplete())
        {
            physicalDevice = device;
            break;
        }
    }

    if (!physicalDevice)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

QueueFamilyIndices VulkanRenderer::findQueueFamilies(vk::PhysicalDevice device)
{
    QueueFamilyIndices indices;

    auto queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            indices.graphicsFamily = i;
        }

        if (device.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface))
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}

// ------------------------------------------------------------
// Logical Device Creation
// ------------------------------------------------------------
void VulkanRenderer::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo(
            vk::DeviceQueueCreateFlags(),
            queueFamily,
            1,
            &queuePriority);
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};

    vk::DeviceCreateInfo createInfo(
        vk::DeviceCreateFlags(),                                                     // flags
        static_cast<uint32_t>(queueCreateInfos.size()),                              // queueCreateInfoCount
        queueCreateInfos.data(),                                                     // pQueueCreateInfos
        enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0, // layerCount
        enableValidationLayers ? validationLayers.data() : nullptr,                  // ppEnabledLayerNames
        static_cast<uint32_t>(deviceExtensions.size()),                              // enabledExtensionCount
        deviceExtensions.data(),                                                     // ppEnabledExtensionNames
        &deviceFeatures                                                              // pEnabledFeatures
    );

    device = physicalDevice.createDevice(createInfo);

    // Initialize device-level functions
    VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

    graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
    presentQueue = device.getQueue(indices.presentFamily.value(), 0);
}

// ------------------------------------------------------------
// Swapchain
// ------------------------------------------------------------
void VulkanRenderer::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo(
        vk::SwapchainCreateFlagsKHR(),
        surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        swapChainSupport.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        VK_TRUE,
        nullptr);

    // Fix: Recalculate queue family indices here
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    if (indices.graphicsFamily != indices.presentFamily)
    {
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    swapChain = device.createSwapchainKHR(createInfo);

    swapChainImages = device.getSwapchainImagesKHR(swapChain);
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanRenderer::cleanupSwapChain()
{
    for (auto framebuffer : swapChainFramebuffers)
    {
        device.destroyFramebuffer(framebuffer);
    }

    // Correct: pass count and pointer
    device.freeCommandBuffers(commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    device.destroyPipeline(graphicsPipeline);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyRenderPass(renderPass);

    for (auto imageView : swapChainImageViews)
    {
        device.destroyImageView(imageView);
    }

    device.destroySwapchainKHR(swapChain);
}

void VulkanRenderer::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    device.waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers(); // Must recreate after new framebuffers
}

// ------------------------------------------------------------
// Image Views
// ------------------------------------------------------------
void VulkanRenderer::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        vk::ImageViewCreateInfo createInfo(
            {},
            swapChainImages[i],
            vk::ImageViewType::e2D,
            swapChainImageFormat,
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        swapChainImageViews[i] = device.createImageView(createInfo);
    }
}

// ------------------------------------------------------------
// Render Pass
// ------------------------------------------------------------
void VulkanRenderer::createRenderPass()
{
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    renderPass = device.createRenderPass(renderPassInfo);
}

// ------------------------------------------------------------
// Graphics Pipeline (precompiled SPIR-V)
// ------------------------------------------------------------
void VulkanRenderer::createGraphicsPipeline()
{
    auto vertSPV = readFile("shaders/triangle.vert.spv");
    auto fragSPV = readFile("shaders/triangle.frag.spv");

    vk::ShaderModuleCreateInfo vertModuleInfo({}, vertSPV.size(), reinterpret_cast<const uint32_t *>(vertSPV.data()));
    vertShaderModule = device.createShaderModule(vertModuleInfo);

    vk::ShaderModuleCreateInfo fragModuleInfo({}, fragSPV.size(), reinterpret_cast<const uint32_t *>(fragSPV.data()));
    fragShaderModule = device.createShaderModule(fragModuleInfo);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo({}, vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main");
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo({}, vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main");

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly({}, vk::PrimitiveTopology::eTriangleList, false);

    vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, swapChainExtent);

    vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampling({}, vk::SampleCountFlagBits::e1, false);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlending({}, false, vk::LogicOp::eCopy, colorBlendAttachment);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        shaderStages,
        &vertexInputInfo,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        nullptr,
        &colorBlending,
        nullptr,
        pipelineLayout,
        renderPass,
        0);

    auto result = device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    graphicsPipeline = result.value;
}

// ------------------------------------------------------------
// Framebuffers
// ------------------------------------------------------------
void VulkanRenderer::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
        vk::FramebufferCreateInfo framebufferInfo(
            {},
            renderPass,
            swapChainImageViews[i],
            swapChainExtent.width,
            swapChainExtent.height,
            1);

        swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
    }
}

// ------------------------------------------------------------
// Command Pool & Buffers
// ------------------------------------------------------------
void VulkanRenderer::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndices.graphicsFamily.value());

    commandPool = device.createCommandPool(poolInfo);
}

void VulkanRenderer::createCommandBuffers()
{
    commandBuffers.resize(swapChainFramebuffers.size());

    vk::CommandBufferAllocateInfo allocInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        static_cast<uint32_t>(commandBuffers.size()));

    commandBuffers = device.allocateCommandBuffers(allocInfo);

    for (size_t i = 0; i < commandBuffers.size(); i++)
    {
        vk::CommandBufferBeginInfo beginInfo;
        commandBuffers[i].begin(beginInfo);

        vk::ClearValue clearColor;
        clearColor.color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});

        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
        renderPassInfo.renderArea.extent = swapChainExtent;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
        commandBuffers[i].draw(3, 1, 0, 0);
        commandBuffers[i].endRenderPass();
        commandBuffers[i].end();
    }
}

// ------------------------------------------------------------
// Sync Objects
// ------------------------------------------------------------
void VulkanRenderer::createSyncObjects()
{
    imageAvailableSemaphores.resize(2);
    renderFinishedSemaphores.resize(2);
    inFlightFences.resize(2);

    vk::SemaphoreCreateInfo semaphoreInfo;
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits::eSignaled);

    for (size_t i = 0; i < 2; i++)
    {
        imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
        renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
        inFlightFences[i] = device.createFence(fenceInfo);
    }
}

// ------------------------------------------------------------
// Draw Frame
// ------------------------------------------------------------
void VulkanRenderer::drawFrame()
{
    // Wait for previous frame
    device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    auto result = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr, &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    device.resetFences(1, &inFlightFences[currentFrame]);

    // Submit command buffer
    vk::SubmitInfo submitInfo;
    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);

    // Present
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    vk::SwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = presentQueue.presentKHR(presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// ------------------------------------------------------------
// Swapchain Support Query Helpers
// ------------------------------------------------------------
SwapChainSupportDetails VulkanRenderer::querySwapChainSupport(vk::PhysicalDevice device)
{
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    return details;
}

vk::SurfaceFormatKHR VulkanRenderer::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
{
    for (const auto &format : availableFormats)
    {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR VulkanRenderer::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
{
    for (const auto &mode : availablePresentModes)
    {
        if (mode == vk::PresentModeKHR::eMailbox)
        {
            return mode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanRenderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    vk::Extent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}
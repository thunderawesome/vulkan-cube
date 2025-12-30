#include "VulkanFrame.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanCommand.h"
#include "VulkanSync.h"
#include "../src/Renderer.h"
#include "../src/Scene.h"
#include "../src/GameObject.h"
#include "../src/RenderStructs.h"
#include <array>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

VulkanFrame::VulkanFrame(const VulkanDevice &device,
                         const VulkanSwapchain &swapchain,
                         const VulkanRenderPass &renderPass,
                         VulkanCommand &command,
                         VulkanSync &sync,
                         Renderer &renderer,
                         uint32_t maxFramesInFlight)
    : deviceRef(device),
      swapchainRef(swapchain),
      renderPassRef(renderPass),
      commandRef(command),
      syncRef(sync),
      rendererRef(renderer),
      maxFramesInFlight(maxFramesInFlight)
{
    auto ext = swapchainRef.getExtent();
    if (ext.height > 0)
        targetAspect = static_cast<float>(ext.width) / static_cast<float>(ext.height);

    // Initial projection matrix (will be updated on resize)
    projMatrix = glm::perspective(glm::radians(45.0f), targetAspect, 0.1f, 100.0f);
    projMatrix[1][1] *= -1; // Flip Y for Vulkan
}

void VulkanFrame::updateTargetAspect()
{
    auto ext = swapchainRef.getExtent();
    if (ext.height > 0)
    {
        targetAspect = static_cast<float>(ext.width) / static_cast<float>(ext.height);
        projMatrix = glm::perspective(glm::radians(45.0f), targetAspect, 0.1f, 100.0f);
        projMatrix[1][1] *= -1;
    }
}

void VulkanFrame::setupViewportAndScissor(vk::CommandBuffer cmd)
{
    auto extent = swapchainRef.getExtent();
    float curW = static_cast<float>(extent.width);
    float curH = static_cast<float>(extent.height);
    float curAspect = (curH > 0.0f) ? (curW / curH) : 1.0f;

    float vpW = curW;
    float vpH = curH;

    if (curAspect > targetAspect)
    {
        vpW = targetAspect * curH;
    }
    else if (curAspect < targetAspect)
    {
        vpH = curW / targetAspect;
    }

    float vpX = (curW - vpW) * 0.5f;
    float vpY = (curH - vpH) * 0.5f;

    vk::Viewport viewport(vpX, vpY, vpW, vpH, 0.0f, 1.0f);
    cmd.setViewport(0, 1, &viewport);

    vk::Rect2D scissor(
        vk::Offset2D(static_cast<int32_t>(std::round(vpX)),
                     static_cast<int32_t>(std::round(vpY))),
        vk::Extent2D(static_cast<uint32_t>(std::round(vpW)),
                     static_cast<uint32_t>(std::round(vpH))));
    cmd.setScissor(0, 1, &scissor);
}

glm::mat4 VulkanFrame::getCameraViewMatrix(const Scene &scene) const
{
    for (const auto *obj : scene.getActiveGameObjects())
    {
        // Camera is the object with no mesh
        if (obj->mesh == nullptr)
        {
            const Transform &t = obj->transform;

            float pitchRad = glm::radians(t.rotation.x);
            float yawRad = glm::radians(t.rotation.y);

            glm::vec3 front;
            front.x = cos(yawRad) * cos(pitchRad);
            front.y = sin(pitchRad);
            front.z = sin(yawRad) * cos(pitchRad);
            front = glm::normalize(front);

            return glm::lookAt(t.position, t.position + front, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }

    return glm::lookAt(glm::vec3(0.0f, 2.0f, 8.0f),
                       glm::vec3(0.0f, 0.0f, 0.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f));
}

FrameResult VulkanFrame::draw(uint32_t &currentFrame, const Scene &scene)
{
    // Wait for fence
    vk::Fence currentFence = syncRef.getInFlightFence(currentFrame);
    (void)deviceRef.getLogicalDevice().waitForFences(1, &currentFence, VK_TRUE, UINT64_MAX);

    // Acquire next image
    uint32_t imageIndex;
    vk::Result result;
    try
    {
        result = deviceRef.getLogicalDevice().acquireNextImageKHR(
            swapchainRef.getSwapchain(),
            UINT64_MAX,
            syncRef.getImageAvailableSemaphore(currentFrame),
            nullptr,
            &imageIndex);
    }
    catch (vk::SystemError &e)
    {
        result = static_cast<vk::Result>(e.code().value());
    }

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        return FrameResult::SwapchainOutOfDate;
    }
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Reset fence
    (void)deviceRef.getLogicalDevice().resetFences(1, &currentFence);

    // Record command buffer
    vk::CommandBuffer cmd = commandRef.getBuffer(currentFrame);
    cmd.reset();

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmd.begin(beginInfo);

    // Begin render pass
    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.01f, 0.01f, 0.02f, 1.0f});
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.renderPass = renderPassRef.get();
    renderPassInfo.framebuffer = swapchainRef.getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.renderArea.extent = swapchainRef.getExtent();
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    setupViewportAndScissor(cmd);

    // --- Prepare Render Context ---
    RenderContext context;
    context.view = getCameraViewMatrix(scene);
    context.proj = projMatrix;

    // Extract camera world position from the inverse view matrix
    glm::mat4 invView = glm::inverse(context.view);
    context.cameraPos = glm::vec3(invView[3]);

    // Simple time tracking for dynamic light position
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Orbiting light logic: circular motion on the XZ plane
    context.lightPos = glm::vec3(sin(time) * 5.0f, 5.0f, cos(time) * 5.0f);

    // Render scene using the context
    auto activeObjects = scene.getActiveGameObjects();
    rendererRef.renderObjects(cmd, activeObjects, context);

    cmd.endRenderPass();
    cmd.end();

    // Submit
    vk::Semaphore waitSemaphores[] = {syncRef.getImageAvailableSemaphore(currentFrame)};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vk::Semaphore signalSemaphores[] = {syncRef.getRenderFinishedSemaphore(imageIndex)};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    deviceRef.getGraphicsQueue().submit(submitInfo, currentFence);

    // Present
    vk::SwapchainKHR currentSwapchain = swapchainRef.getSwapchain();
    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &currentSwapchain;
    presentInfo.pImageIndices = &imageIndex;

    try
    {
        result = deviceRef.getPresentQueue().presentKHR(presentInfo);
    }
    catch (vk::SystemError &e)
    {
        result = static_cast<vk::Result>(e.code().value());
    }

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        return FrameResult::SwapchainOutOfDate;
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
    return FrameResult::Success;
}
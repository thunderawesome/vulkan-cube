#include "VulkanFrame.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanCommand.h"
#include "VulkanSync.h"
#include "Mesh.h"

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

VulkanFrame::VulkanFrame(const VulkanDevice &device,
                         const VulkanSwapchain &swapchain,
                         const VulkanRenderPass &renderPass,
                         const VulkanGraphicsPipeline &pipeline,
                         VulkanCommand &command,
                         VulkanSync &sync,
                         Mesh &mesh,
                         uint32_t maxFramesInFlight)
    : deviceRef(device),
      swapchainRef(swapchain),
      renderPassRef(renderPass),
      pipelineRef(pipeline),
      commandRef(command),
      syncRef(sync),
      meshRef(mesh),
      maxFramesInFlight(maxFramesInFlight)
{
    auto ext = swapchainRef.getExtent();
    if (ext.height > 0)
        targetAspect = static_cast<float>(ext.width) / static_cast<float>(ext.height);
}

FrameResult VulkanFrame::draw(uint32_t &currentFrame)
{
    (void)deviceRef.getLogicalDevice().waitForFences(
        1, &syncRef.getInFlightFence(currentFrame), VK_TRUE, UINT64_MAX);

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

    (void)deviceRef.getLogicalDevice().resetFences(1, &syncRef.getInFlightFence(currentFrame));

    vk::CommandBuffer cmd = commandRef.getBuffer(currentFrame);
    cmd.reset();

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmd.begin(beginInfo);

    // Clear both color AND depth attachments
    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
    clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass = renderPassRef.get();
    renderPassInfo.framebuffer = swapchainRef.getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.renderArea.extent = swapchainRef.getExtent();
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelineRef.get());

    // Preserve the original aspect ratio by letterboxing when the window is non-uniformly resized.
    auto extent = swapchainRef.getExtent();
    float curW = static_cast<float>(extent.width);
    float curH = static_cast<float>(extent.height);
    float curAspect = (curH > 0.0f) ? (curW / curH) : 1.0f;

    float vpW = curW;
    float vpH = curH;
    if (curAspect > targetAspect)
    {
        // window is wider than target -> limit width
        vpW = targetAspect * curH;
    }
    else if (curAspect < targetAspect)
    {
        // window is taller than target -> limit height
        vpH = curW / targetAspect;
    }

    float vpX = (curW - vpW) * 0.5f;
    float vpY = (curH - vpH) * 0.5f;

    vk::Viewport viewport(vpX, vpY, vpW, vpH, 0.0f, 1.0f);
    cmd.setViewport(0, 1, &viewport);

    // scissor uses integer extents
    vk::Offset2D scOff(static_cast<int32_t>(std::round(vpX)), static_cast<int32_t>(std::round(vpY)));
    vk::Extent2D scExt(static_cast<uint32_t>(std::round(vpW)), static_cast<uint32_t>(std::round(vpH)));
    vk::Rect2D scissor(scOff, scExt);
    cmd.setScissor(0, 1, &scissor);

    // bind mesh vertex buffer and draw
    // compute a modest static rotation and a simple perspective view
    float aspect = (extent.height > 0) ? (static_cast<float>(extent.width) / static_cast<float>(extent.height)) : 1.0f;

    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(-25.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);
    proj[1][1] *= -1; // GLM's clip coordinates vs Vulkan
    glm::mat4 mvp = proj * view * model;

    // push the mvp via push-constants
    cmd.pushConstants(pipelineRef.getLayout(), vk::ShaderStageFlagBits::eVertex, 0, static_cast<uint32_t>(sizeof(glm::mat4)), &mvp);

    meshRef.bind(cmd);
    meshRef.draw(cmd);

    cmd.endRenderPass();
    cmd.end();

    vk::SubmitInfo submitInfo;
    vk::Semaphore waitSemaphores[] = {syncRef.getImageAvailableSemaphore(currentFrame)};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vk::Semaphore signalSemaphores[] = {syncRef.getRenderFinishedSemaphore(imageIndex)};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    deviceRef.getGraphicsQueue().submit(submitInfo, syncRef.getInFlightFence(currentFrame));

    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    vk::SwapchainKHR swapChains[] = {swapchainRef.getSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
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
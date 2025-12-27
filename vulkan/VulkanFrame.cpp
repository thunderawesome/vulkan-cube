#include "VulkanFrame.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanCommand.h"
#include "VulkanSync.h"

#include <array>

VulkanFrame::VulkanFrame(const VulkanDevice &device,
                         const VulkanSwapchain &swapchain,
                         const VulkanRenderPass &renderPass,
                         const VulkanGraphicsPipeline &pipeline,
                         VulkanCommand &command,
                         VulkanSync &sync,
                         uint32_t maxFramesInFlight)
    : deviceRef(device),
      swapchainRef(swapchain),
      renderPassRef(renderPass),
      pipelineRef(pipeline),
      commandRef(command),
      syncRef(sync),
      maxFramesInFlight(maxFramesInFlight)
{
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

    vk::ClearValue clearColor;
    clearColor.color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});

    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass = renderPassRef.get();
    renderPassInfo.framebuffer = swapchainRef.getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassInfo.renderArea.extent = swapchainRef.getExtent();
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    cmd.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelineRef.get());

    vk::Viewport viewport(
        0.0f, 0.0f,
        static_cast<float>(swapchainRef.getExtent().width),
        static_cast<float>(swapchainRef.getExtent().height),
        0.0f, 1.0f);
    cmd.setViewport(0, 1, &viewport);

    vk::Rect2D scissor({0, 0}, swapchainRef.getExtent());
    cmd.setScissor(0, 1, &scissor);

    cmd.draw(3, 1, 0, 0);

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
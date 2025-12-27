#include "VulkanFrame.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanRenderPass.h"
#include "VulkanCommand.h"
#include "VulkanSync.h"
#include "src/Mesh.h"
#include "src/GameObject.h"
#include "src/Material.h"

#include <array>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

VulkanFrame::VulkanFrame(const VulkanDevice &device,
                         const VulkanSwapchain &swapchain,
                         const VulkanRenderPass &renderPass,
                         VulkanCommand &command,
                         VulkanSync &sync,
                         uint32_t maxFramesInFlight)
    : deviceRef(device),
      swapchainRef(swapchain),
      renderPassRef(renderPass),
      commandRef(command),
      syncRef(sync),
      maxFramesInFlight(maxFramesInFlight)
{
    auto ext = swapchainRef.getExtent();
    if (ext.height > 0)
        targetAspect = static_cast<float>(ext.width) / static_cast<float>(ext.height);
}

void VulkanFrame::addGameObject(GameObject *obj)
{
    if (obj)
        gameObjects.push_back(obj);
}

void VulkanFrame::clearGameObjects()
{
    gameObjects.clear();
}

void VulkanFrame::updateTargetAspect()
{
    auto ext = swapchainRef.getExtent();
    if (ext.height > 0)
        targetAspect = static_cast<float>(ext.width) / static_cast<float>(ext.height);
}

void VulkanFrame::renderObjects(vk::CommandBuffer cmd,
                                const glm::mat4 &view,
                                const glm::mat4 &proj)
{
    // Batch objects by material to minimize pipeline switches
    std::unordered_map<Material *, std::vector<GameObject *>> batchedObjects;

    for (GameObject *obj : gameObjects)
    {
        if (obj && obj->enabled && obj->mesh && obj->material)
        {
            batchedObjects[obj->material].push_back(obj);
        }
    }

    // Render each material batch (C++11 compatible iteration)
    for (auto it = batchedObjects.begin(); it != batchedObjects.end(); ++it)
    {
        Material *material = it->first;
        std::vector<GameObject *> &objects = it->second;

        // Bind pipeline once per material
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, material->getPipeline());

        // Render all objects using this material
        for (GameObject *obj : objects)
        {
            glm::mat4 model = obj->transform.getMatrix();
            glm::mat4 mvp = proj * view * model;

            cmd.pushConstants(material->getLayout(),
                              vk::ShaderStageFlagBits::eVertex,
                              0, sizeof(glm::mat4), &mvp);

            obj->mesh->bind(cmd);
            obj->mesh->draw(cmd);
        }
    }
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

    // Setup viewport and scissor
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

    vk::Offset2D scOff(static_cast<int32_t>(std::round(vpX)), static_cast<int32_t>(std::round(vpY)));
    vk::Extent2D scExt(static_cast<uint32_t>(std::round(vpW)), static_cast<uint32_t>(std::round(vpH)));
    vk::Rect2D scissor(scOff, scExt);
    cmd.setScissor(0, 1, &scissor);

    // Compute view and projection matrices
    glm::mat4 view = glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), targetAspect, 0.1f, 100.0f);
    proj[1][1] *= -1;

    // Render all objects (batched by material)
    renderObjects(cmd, view, proj);

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
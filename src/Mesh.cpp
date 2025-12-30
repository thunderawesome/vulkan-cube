#include "Mesh.h"
#include "../vulkan/VulkanDevice.h"
#include "VulkanUtils.h"

#include <cstring>
#include <stdexcept>

Mesh::Mesh(const VulkanDevice &device, const std::vector<Vertex> &vertices)
    : deviceRef(device)
{
    createVertexBuffer(vertices);
}

Mesh::~Mesh()
{
    if (vertexBuffer)
        deviceRef.getLogicalDevice().destroyBuffer(vertexBuffer);
    if (vertexMemory)
        deviceRef.getLogicalDevice().freeMemory(vertexMemory);
}

void Mesh::bind(vk::CommandBuffer cmd) const
{
    vk::DeviceSize offsets[] = {0};
    cmd.bindVertexBuffers(0, 1, &vertexBuffer, offsets);
}

void Mesh::draw(vk::CommandBuffer cmd) const
{
    cmd.draw(vertexCount, 1, 0, 0);
}

uint32_t Mesh::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
    auto memProperties = deviceRef.getPhysicalDevice().getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Mesh::createVertexBuffer(const std::vector<Vertex> &vertices)
{
    vertexCount = static_cast<uint32_t>(vertices.size());
    VulkanUtils::uploadBuffer(deviceRef,
                              vk::BufferUsageFlagBits::eVertexBuffer,
                              vertices,
                              vertexBuffer,
                              vertexMemory);
}
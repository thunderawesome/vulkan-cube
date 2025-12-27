#include "Mesh.h"
#include "VulkanDevice.h"

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

std::vector<Mesh::Vertex> Mesh::createCubeVertices()
{
    std::vector<Vertex> verts;
    verts.reserve(36);

    std::array<glm::vec3, 8> positions = {
        glm::vec3(-0.5f, -0.5f, -0.5f), // 0
        glm::vec3(0.5f, -0.5f, -0.5f),  // 1
        glm::vec3(0.5f, 0.5f, -0.5f),   // 2
        glm::vec3(-0.5f, 0.5f, -0.5f),  // 3
        glm::vec3(-0.5f, -0.5f, 0.5f),  // 4
        glm::vec3(0.5f, -0.5f, 0.5f),   // 5
        glm::vec3(0.5f, 0.5f, 0.5f),    // 6
        glm::vec3(-0.5f, 0.5f, 0.5f)    // 7
    };

    std::array<glm::vec3, 8> colors = {
        glm::vec3(1.0f, 0.0f, 0.0f), // red
        glm::vec3(0.0f, 1.0f, 0.0f), // green
        glm::vec3(0.0f, 0.0f, 1.0f), // blue
        glm::vec3(1.0f, 1.0f, 0.0f), // yellow
        glm::vec3(1.0f, 0.0f, 1.0f), // magenta
        glm::vec3(0.0f, 1.0f, 1.0f), // cyan
        glm::vec3(1.0f, 0.5f, 0.0f), // orange
        glm::vec3(1.0f, 1.0f, 1.0f)  // white
    };

    // Corrected indices with proper counter-clockwise winding when viewed from outside
    const uint32_t idxs[36] = {
        // back face (facing -Z)
        0, 3, 2, 2, 1, 0,
        // front face (facing +Z)
        4, 5, 6, 6, 7, 4,
        // left face (facing -X)
        4, 7, 3, 3, 0, 4,
        // right face (facing +X)
        1, 2, 6, 6, 5, 1,
        // bottom face (facing -Y)
        4, 0, 1, 1, 5, 4,
        // top face (facing +Y)
        3, 7, 6, 6, 2, 3};

    for (size_t i = 0; i < 36; ++i)
    {
        uint32_t idx = idxs[i];
        verts.push_back({positions[idx], colors[idx]});
    }

    return verts;
}

uint32_t Mesh::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const
{
    auto memProperties = deviceRef.getPhysicalDevice().getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Mesh::createVertexBuffer(const std::vector<Vertex> &vertices)
{
    vertexCount = static_cast<uint32_t>(vertices.size());
    vk::DeviceSize bufferSize = sizeof(Vertex) * vertexCount;

    // staging buffer
    vk::BufferCreateInfo bufferInfo({}, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive);
    auto device = deviceRef.getLogicalDevice();
    vk::Buffer stagingBuffer = device.createBuffer(bufferInfo);

    auto memReq = device.getBufferMemoryRequirements(stagingBuffer);
    vk::MemoryAllocateInfo allocInfo(memReq.size, findMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
    vk::DeviceMemory stagingMemory = device.allocateMemory(allocInfo);
    device.bindBufferMemory(stagingBuffer, stagingMemory, 0);

    // copy vertex data
    void *data = device.mapMemory(stagingMemory, 0, bufferSize);
    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    device.unmapMemory(stagingMemory);

    // create device local buffer
    vk::BufferCreateInfo vertexBufferInfo({}, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive);
    vertexBuffer = device.createBuffer(vertexBufferInfo);

    auto vertReq = device.getBufferMemoryRequirements(vertexBuffer);
    vk::MemoryAllocateInfo vertAlloc(vertReq.size, findMemoryType(vertReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    vertexMemory = device.allocateMemory(vertAlloc);
    device.bindBufferMemory(vertexBuffer, vertexMemory, 0);

    // copy staging -> vertex buffer using a temporary command pool and buffer
    vk::CommandPoolCreateInfo poolInfo({}, deviceRef.getGraphicsQueueFamily());
    vk::CommandPool cmdPool = device.createCommandPool(poolInfo);

    vk::CommandBufferAllocateInfo allocCmdInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1);
    auto cmdBufs = device.allocateCommandBuffers(allocCmdInfo);
    vk::CommandBuffer cmd = cmdBufs[0];

    vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmd.begin(beginInfo);
    vk::BufferCopy copyRegion(0, 0, bufferSize);
    cmd.copyBuffer(stagingBuffer, vertexBuffer, 1, &copyRegion);
    cmd.end();

    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &cmd);
    deviceRef.getGraphicsQueue().submit(submitInfo, {});
    deviceRef.getGraphicsQueue().waitIdle();

    device.freeCommandBuffers(cmdPool, cmd);
    device.destroyCommandPool(cmdPool);

    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingMemory);
}

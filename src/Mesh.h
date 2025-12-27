#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include "Primitive.h"

class VulkanDevice;

class Mesh
{
public:
    Mesh(const VulkanDevice &device, const std::vector<Vertex> &vertices);
    ~Mesh();

    // Delete copy operations
    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    void bind(vk::CommandBuffer cmd) const;
    void draw(vk::CommandBuffer cmd) const;

    uint32_t getVertexCount() const { return vertexCount; }

private:
    const VulkanDevice &deviceRef;
    vk::Buffer vertexBuffer = {};
    vk::DeviceMemory vertexMemory = {};
    uint32_t vertexCount = 0;

    void createVertexBuffer(const std::vector<Vertex> &vertices);
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
};
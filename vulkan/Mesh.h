#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <array>

class VulkanDevice;

class Mesh
{
public:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;

        static vk::VertexInputBindingDescription binding()
        {
            vk::VertexInputBindingDescription bd;
            bd.binding = 0;
            bd.stride = sizeof(Vertex);
            bd.inputRate = vk::VertexInputRate::eVertex;
            return bd;
        }

        static std::array<vk::VertexInputAttributeDescription, 2> attributes()
        {
            std::array<vk::VertexInputAttributeDescription, 2> attrs{};
            attrs[0].binding = 0;
            attrs[0].location = 0;
            attrs[0].format = vk::Format::eR32G32B32Sfloat;
            attrs[0].offset = offsetof(Vertex, pos);

            attrs[1].binding = 0;
            attrs[1].location = 1;
            attrs[1].format = vk::Format::eR32G32B32Sfloat;
            attrs[1].offset = offsetof(Vertex, color);

            return attrs;
        }
    };

    Mesh(const VulkanDevice &device, const std::vector<Vertex> &vertices);
    ~Mesh();

    void bind(vk::CommandBuffer cmd) const;
    void draw(vk::CommandBuffer cmd) const;

    static std::vector<Vertex> createCubeVertices();

private:
    const VulkanDevice &deviceRef;
    vk::Buffer vertexBuffer = {};
    vk::DeviceMemory vertexMemory = {};
    uint32_t vertexCount = 0;

    void createVertexBuffer(const std::vector<Vertex> &vertices);
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;
};

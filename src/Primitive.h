#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <array>

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

namespace Primitives
{
    std::vector<Vertex> createCube();
    std::vector<Vertex> createTriangle();
    // Add more primitives as needed
    // std::vector<Vertex> createSphere(uint32_t segments);
    // std::vector<Vertex> createPlane();
}
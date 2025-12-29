#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <array>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal; // Added normal

    static vk::VertexInputBindingDescription binding()
    {
        vk::VertexInputBindingDescription bd;
        bd.binding = 0;
        bd.stride = sizeof(Vertex);
        bd.inputRate = vk::VertexInputRate::eVertex;
        return bd;
    }

    static std::array<vk::VertexInputAttributeDescription, 3> attributes()
    { // Changed to 3
        std::array<vk::VertexInputAttributeDescription, 3> attrs{};

        attrs[0].location = 0;
        attrs[0].format = vk::Format::eR32G32B32Sfloat;
        attrs[0].offset = offsetof(Vertex, pos);

        attrs[1].location = 1;
        attrs[1].format = vk::Format::eR32G32B32Sfloat;
        attrs[1].offset = offsetof(Vertex, color);

        // New Normal Attribute
        attrs[2].location = 2;
        attrs[2].format = vk::Format::eR32G32B32Sfloat;
        attrs[2].offset = offsetof(Vertex, normal);

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
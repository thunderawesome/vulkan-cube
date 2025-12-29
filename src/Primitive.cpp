#include "Primitive.h"
#include <array>

namespace Primitives
{
    std::vector<Vertex> createCube()
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

        // Normals for each of the 6 faces
        std::array<glm::vec3, 6> faceNormals = {
            glm::vec3(0.0f, 0.0f, -1.0f), // back
            glm::vec3(0.0f, 0.0f, 1.0f),  // front
            glm::vec3(-1.0f, 0.0f, 0.0f), // left
            glm::vec3(1.0f, 0.0f, 0.0f),  // right
            glm::vec3(0.0f, -1.0f, 0.0f), // bottom
            glm::vec3(0.0f, 1.0f, 0.0f)   // top
        };

        for (size_t i = 0; i < 36; ++i)
        {
            uint32_t idx = idxs[i];
            // Every 6 vertices belong to one face, so we select the normal based on i / 6
            glm::vec3 normal = faceNormals[i / 6];
            verts.push_back({positions[idx], colors[idx], normal});
        }

        return verts;
    }

    std::vector<Vertex> createTriangle()
    {
        // For a single triangle on the XY plane, the normal points toward +Z
        glm::vec3 normal = {0.0f, 0.0f, 1.0f};
        return {
            {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, normal},
            {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, normal},
            {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, normal}};
    }
}
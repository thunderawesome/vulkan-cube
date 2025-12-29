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
            glm::vec3(1.0f, 0.0f, 0.0f), // Red
            glm::vec3(0.0f, 1.0f, 0.0f), // Green
            glm::vec3(0.0f, 0.0f, 1.0f), // Blue
            glm::vec3(1.0f, 1.0f, 0.0f), // Yellow
            glm::vec3(1.0f, 0.0f, 1.0f), // Magenta
            glm::vec3(0.0f, 1.0f, 1.0f), // Cyan
            glm::vec3(1.0f, 0.5f, 0.0f), // Orange
            glm::vec3(0.5f, 0.0f, 1.0f)  // Purple
        };

        std::array<glm::vec3, 6> faceNormals = {
            glm::vec3(0.0f, 0.0f, -1.0f), // Back
            glm::vec3(0.0f, 0.0f, 1.0f),  // Front
            glm::vec3(-1.0f, 0.0f, 0.0f), // Left
            glm::vec3(1.0f, 0.0f, 0.0f),  // Right
            glm::vec3(0.0f, -1.0f, 0.0f), // Bottom
            glm::vec3(0.0f, 1.0f, 0.0f)   // Top
        };

        const uint32_t idxs[36] = {
            0, 3, 2, 2, 1, 0, // Back
            4, 5, 6, 6, 7, 4, // Front
            4, 7, 3, 3, 0, 4, // Left
            1, 2, 6, 6, 5, 1, // Right
            4, 0, 1, 1, 5, 4, // Bottom
            3, 7, 6, 6, 2, 3  // Top
        };

        // UVs for a quad (two triangles)
        std::array<glm::vec2, 6> quadUVs = {
            glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f),
            glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f)};

        for (int i = 0; i < 36; ++i)
        {
            uint32_t posIdx = idxs[i];
            int face = i / 6;
            int uvIdx = i % 6;

            verts.push_back({positions[posIdx],
                             colors[posIdx],
                             faceNormals[face],
                             quadUVs[uvIdx]});
        }

        return verts;
    }

    std::vector<Vertex> createTriangle()
    {
        glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
        return {
            // Position            // Color (RGB)        // Normal  // UV
            {{-0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, normal, {0.0f, 1.0f}}, // Red corner
            {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, normal, {1.0f, 1.0f}},  // Green corner
            {{0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, normal, {0.5f, 0.0f}}  // Blue corner
        };
    }
}
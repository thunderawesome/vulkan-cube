#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Mesh;
class Material;

struct Transform
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // Euler angles in degrees
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 getMatrix() const
    {
        glm::mat4 model = glm::mat4(1.0f);

        // Apply transformations: translate -> rotate -> scale
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);

        return model;
    }
};

struct GameObject
{
    Mesh *mesh = nullptr;
    Material *material = nullptr; // Add material
    Transform transform;
    bool enabled = true; // Allow disabling objects

    GameObject(Mesh *m, Material *mat)
        : mesh(m), material(mat) {}

    GameObject(Mesh *m, Material *mat, const Transform &t)
        : mesh(m), material(mat), transform(t) {}
};
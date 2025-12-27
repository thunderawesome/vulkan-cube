#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Mesh;

struct Transform
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // Euler angles in degrees
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 getMatrix() const
    {
        glm::mat4 model = glm::mat4(1.0f);

        // Apply transformations: scale -> rotate -> translate
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
    Transform transform;

    GameObject(Mesh *m) : mesh(m) {}
    GameObject(Mesh *m, const Transform &t) : mesh(m), transform(t) {}
};
#pragma once
#include "Transform.h"
#include <functional>

class Mesh;
class Material;

struct GameObject;
using UpdateFunc = std::function<void(GameObject &, float)>;

struct GameObject
{
    Mesh *mesh = nullptr;
    Material *material = nullptr;
    Transform transform;
    bool enabled = true;

    // Optional update behavior - nullptr means no update
    UpdateFunc updateFunc = nullptr;

    GameObject(Mesh *m, Material *mat)
        : mesh(m), material(mat) {}

    GameObject(Mesh *m, Material *mat, const Transform &t)
        : mesh(m), material(mat), transform(t) {}

    void update(float deltaTime)
    {
        if (updateFunc)
            updateFunc(*this, deltaTime);
    }
};
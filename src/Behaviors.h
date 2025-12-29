#pragma once
#include "GameObject.h"
#include <glm/glm.hpp>
#include <vector>
#include <cmath>

namespace Behaviors
{
    // Rotate continuously around an axis
    inline UpdateFunc rotate(const glm::vec3 &degreesPerSecond)
    {
        return [degreesPerSecond](GameObject &obj, float dt)
        {
            obj.transform.rotation += degreesPerSecond * dt;
        };
    }

    // Orbit around a point
    inline UpdateFunc orbit(const glm::vec3 &center, float radius, float degreesPerSecond)
    {
        return [center, radius, degreesPerSecond, angle = 0.0f](GameObject &obj, float dt) mutable
        {
            angle += degreesPerSecond * dt;
            float rad = glm::radians(angle);
            obj.transform.position = center + glm::vec3(
                                                  radius * std::cos(rad),
                                                  0.0f,
                                                  radius * std::sin(rad));
        };
    }

    // Bounce up and down
    inline UpdateFunc bounce(float amplitude, float frequency)
    {
        return [amplitude, frequency, time = 0.0f](GameObject &obj, float dt) mutable
        {
            time += dt;
            obj.transform.position.y = amplitude * std::sin(frequency * time);
        };
    }

    // Combine multiple behaviors
    inline UpdateFunc combine(std::vector<UpdateFunc> funcs)
    {
        return [funcs = std::move(funcs)](GameObject &obj, float dt)
        {
            for (const auto &f : funcs)
                if (f)
                    f(obj, dt);
        };
    }
}
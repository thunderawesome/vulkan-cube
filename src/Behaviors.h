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

    // Orbit around a point (Updated to be more flexible)
    inline UpdateFunc orbit(const glm::vec3 &center, float radius, float degreesPerSecond)
    {
        return [center, radius, degreesPerSecond, angle = 0.0f](GameObject &obj, float dt) mutable
        {
            angle += degreesPerSecond * dt;
            float rad = glm::radians(angle);
            obj.transform.position = center + glm::vec3(
                                                  radius * std::cos(rad),
                                                  obj.transform.position.y, // Keep current height
                                                  radius * std::sin(rad));
        };
    }

    // Bounce up and down relative to the starting position
    inline UpdateFunc bounce(float amplitude, float frequency)
    {
        // We use a flag to capture the starting Y position on the first frame
        return [amplitude, frequency, time = 0.0f, initialY = 0.0f, started = false](GameObject &obj, float dt) mutable
        {
            if (!started)
            {
                initialY = obj.transform.position.y;
                started = true;
            }

            time += dt;
            // Use += or add to initialY to ensure we don't overwrite world position
            obj.transform.position.y = initialY + (amplitude * std::sin(frequency * time));
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
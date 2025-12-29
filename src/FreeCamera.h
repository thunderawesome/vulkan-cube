#pragma once

#include "Behaviors.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace Behaviors
{
    inline UpdateFunc freeCamera(GLFWwindow *window, float moveSpeed = 6.0f, float lookSensitivity = 0.15f)
    {
        struct State
        {
            glm::vec3 position = glm::vec3(0.0f, 2.0f, 8.0f);
            float yaw = -90.0f;
            float pitch = -15.0f;
            bool firstMouse = true;
            double lastX = 0.0;
            double lastY = 0.0;
        };

        auto state = std::make_shared<State>();

        // Capture the mouse
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        return [window, moveSpeed, lookSensitivity, state](GameObject &obj, float dt) mutable
        {
            // --- Mouse Look ---
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            if (state->firstMouse)
            {
                state->lastX = xpos;
                state->lastY = ypos;
                state->firstMouse = false;
            }

            float xoffset = static_cast<float>(xpos - state->lastX);
            float yoffset = static_cast<float>(state->lastY - ypos); // Y inverted
            state->lastX = xpos;
            state->lastY = ypos;

            xoffset *= lookSensitivity;
            yoffset *= lookSensitivity;

            state->yaw += xoffset;
            state->pitch += yoffset;

            // Clamp pitch
            if (state->pitch > 89.0f)
                state->pitch = 89.0f;
            if (state->pitch < -89.0f)
                state->pitch = -89.0f;

            // Compute forward direction
            glm::vec3 front;
            front.x = cos(glm::radians(state->yaw)) * cos(glm::radians(state->pitch));
            front.y = sin(glm::radians(state->pitch));
            front.z = sin(glm::radians(state->yaw)) * cos(glm::radians(state->pitch));
            front = glm::normalize(front);

            // --- WASD + Space/Shift Movement ---
            glm::vec3 moveDir(0.0f);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                moveDir += front;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                moveDir -= front;

            glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                moveDir += right;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                moveDir -= right;

            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                moveDir.y += 1.0f;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                moveDir.y -= 1.0f;

            if (glm::dot(moveDir, moveDir) > 0.0f)
            {
                moveDir = glm::normalize(moveDir) * moveSpeed * dt;
                state->position += moveDir;
            }

            // Update GameObject transform
            obj.transform.position = state->position;
            obj.transform.rotation = glm::vec3(state->pitch, state->yaw, 0.0f);
        };
    }
}
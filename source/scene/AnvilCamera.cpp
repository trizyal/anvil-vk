// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilCamera.h"

#include <algorithm>

#include "AnvilInput.h"
#include "glm/gtc/matrix_transform.hpp"

AnvilCamera::AnvilCamera(const glm::vec3 inStartPosition)
{
    position = inStartPosition;
    yawDegree = -90.0f; // Look straight ahead (negative Z)
    pitchDegree = 0.0f;

    updateCameraVectors();
}

glm::mat4 AnvilCamera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 AnvilCamera::getProjectionMatrix(const float aspectRatio) const
{
    glm::mat4 projection = glm::perspective(glm::radians(fovDegrees), aspectRatio, 0.1f, 100.0f);

    // Flip Y for Vulkan's coordinate system
    projection[1][1] *= -1.0f;

    return projection;
}

void AnvilCamera::updateCamera(float deltaTime)
{
    float velocity = cameraSpeed * deltaTime;

    // Keyboard Movement
    if (AnvilInput::IsKeyPressed(GLFW_KEY_W)) position += front * velocity;
    if (AnvilInput::IsKeyPressed(GLFW_KEY_S)) position -= front * velocity;
    if (AnvilInput::IsKeyPressed(GLFW_KEY_A)) position -= right * velocity;
    if (AnvilInput::IsKeyPressed(GLFW_KEY_D)) position += right * velocity;

    // Vertical movement
    // if (AnvilInput::IsKeyPressed(GLFW_KEY_E)) position += up * velocity;
    if (AnvilInput::IsKeyPressed(GLFW_KEY_E)) position += worldUp * velocity;
    // if (AnvilInput::IsKeyPressed(GLFW_KEY_Q)) position -= up * velocity;
    if (AnvilInput::IsKeyPressed(GLFW_KEY_Q)) position -= worldUp * velocity;

    // Sprinting
    if (AnvilInput::IsKeyPressed(GLFW_KEY_LEFT_SHIFT)) velocity *= 2.5f;

    // Mouse Look
    // Only rotate if the mouse button is held down
    if (AnvilInput::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
    {
        // Hide and lock the cursor
        AnvilInput::SetCursorMode(GLFW_CURSOR_DISABLED);

        glm::vec2 mouseDelta = AnvilInput::GetMouseDelta();
        yawDegree += mouseDelta.x * mouseSensitivity;
        pitchDegree += mouseDelta.y * mouseSensitivity;

        // Constrain pitch so the screen does not flip upside down
        pitchDegree = std::clamp(pitchDegree, -89.0f, 89.0f);

        updateCameraVectors();
    }
    else
    {
        // Release the cursor when the user lets go of right-click
        AnvilInput::SetCursorMode(GLFW_CURSOR_NORMAL);
    }
}

void AnvilCamera::updateCameraVectors()
{
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yawDegree)) * cos(glm::radians(pitchDegree));
    newFront.y = sin(glm::radians(pitchDegree));
    newFront.z = sin(glm::radians(yawDegree)) * cos(glm::radians(pitchDegree));

    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

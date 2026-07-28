// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_CAMERA_H
#define ANVIL_VK_CAMERA_H

#include <glm/glm.hpp>

class AnvilCamera
{
public:
    static constexpr glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    static constexpr glm::vec3 defaultFront = glm::vec3(0.0f, 0.0f, -1.0f);
    static constexpr glm::vec3 defaultRight = glm::vec3(1.0f, 0.0f, 0.0f);
    static constexpr glm::vec3 defaultStartPosition = glm::vec3(0.0f, 0.0f, 5.0f);

    AnvilCamera(glm::vec3 inStartPosition = defaultStartPosition);

private:
    glm::vec3 position = defaultStartPosition;
    glm::vec3 front = defaultFront;
    glm::vec3 up = worldUp;
    glm::vec3 right = defaultRight;

    float yawDegree;
    float pitchDegree;

public:
    float cameraSpeed = 5.0f;
    float mouseSensitivity = 0.2f;
    float fovDegrees = 70.0f;

    void updateCamera(float deltaTime);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

private:
    void updateCameraVectors();
};

#endif //ANVIL_VK_CAMERA_H

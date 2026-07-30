// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_CAMERA_H
#define ANVIL_VK_CAMERA_H

/**
 * @file AnvilCamera.h
 * @brief First-person 3D camera abstraction producing Vulkan-compatible view and projection matrices.
 */

#include <glm/glm.hpp>

/**
 * @brief Fixed world-space UP vector.
 *
 * By default UP is positive Y.
 */
constexpr glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);

/**
 * @brief Initial forward look direction.
 *
 * Set as negative Z.
 */
constexpr glm::vec3 WORLD_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);

/**
 * @brief Initial right-pointing axis.
 *
 * Set as positive X.
 */
constexpr glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);

/**
 * @brief Free-look 3D camera managing spatial vectors, Euler angles, and matrix generation.
 *
 * Generates Vulkan-compatible view and projection matrices.
 * Designed to be updated per-frame based on input deltas and frame timing.
 *
 * @note This class is non-copyable to prevent accidental duplication of camera states,
 * but is movable so it can be stored in containers or transferred between scopes.
 *
 * @todo Need to implement setters for camera speed and other things. Speed setters can be with scale or value.
 */
class AnvilCamera
{
public:

    static constexpr glm::vec3 defaultStartPosition = glm::vec3(0.0f, 0.0f, 5.0f);
    static constexpr float defaultCameraSpeed = 5.0f;
    static constexpr float defaultCameraSensitivity = 0.2f;
    static constexpr float defaultCameraFOVDegrees = 70.0f;

    /**
     * @brief Constructs a camera at the specified world-space origin.
     *
     * Because the parameter has a default value, this signature serves as both the default
     * constructor and an explicit single-argument position constructor.
     *
     * @param inStartPosition Starting camera world coordinates. Defaults to 0, 0, 5.
     */
    explicit AnvilCamera(glm::vec3 inStartPosition = defaultStartPosition);
    ~AnvilCamera() = default;

    AnvilCamera(const AnvilCamera&) = delete;
    AnvilCamera& operator=(const AnvilCamera&) = delete;

    AnvilCamera(AnvilCamera&&) noexcept = default;
    AnvilCamera& operator=(AnvilCamera&&) noexcept = default;

private:
    glm::vec3 position = defaultStartPosition;
    glm::vec3 front = WORLD_FRONT;
    glm::vec3 up = WORLD_UP;
    glm::vec3 right = WORLD_RIGHT;

    float yawDegree;
    float pitchDegree;

public:
    float cameraSpeed = defaultCameraSpeed;
    float cameraSensitivity = defaultCameraSensitivity;
    float fovDegrees = defaultCameraFOVDegrees;

    /**
     * @brief Updates camera position and orientation based on frame timing and input.
     *
     * Should be called once per frame prior to querying matrices for rendering.
     *
     * @param deltaTime Time elapsed in seconds since the previous frame.
     */
    void updateCamera(float deltaTime);

    /**
     * @brief Calculates and returns the current view matrix.
     *
     * Transforms coordinates from world space to camera view space using the current
     * position, forward vector, and up vector.
     *
     * @return 4x4 view matrix.
     */
    [[nodiscard]] glm::mat4 getViewMatrix() const;

    /**
     * @brief Calculates and returns a Vulkan-compatible perspective projection matrix.
     *
     * Transforms coordinates from view space to clip space. Accounted for Vulkan's inverted
     * Y-coordinate clip space where necessary.
     *
     * @param aspectRatio Viewport width divided by viewport height.
     * @return 4x4 perspective projection matrix.
     */
    [[nodiscard]] glm::mat4 getProjectionMatrix(float aspectRatio) const;

private:
    void updateCameraVectors();
};

#endif //ANVIL_VK_CAMERA_H

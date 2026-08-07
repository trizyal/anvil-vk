// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SCENE_H
#define ANVIL_VK_SCENE_H

/**
 * @file Scene.h
 * @brief Scene abstraction holding Material equivalents to a Scene.
 */

#include <glm/glm.hpp>

#include "GPUBuffer.h"

class VulkanContext;

/**
 * @brief Struct to hold generic single directionally lit scene lighting data.
 *
 * @note Strictly 16-byte aligned for Vulkan UBO rules
 */
struct GPUSceneData
{
    glm::vec4 lightDirection;   /**< w = unused/padding */
    glm::vec4 lightColor;       /**< w = intensity */
    glm::vec4 ambientColor;     /**< w = unused/padding */
};

/**
 * @brief
 *
 * @note This class in non-copyable but moving is allowed.
 */
class Scene
{
public:
    Scene() = default;
    ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

private:
    VulkanContext* ptrContext = nullptr;
    bool isDirty = true;

public:
    GPUSceneData data{};
    GPUBuffer sceneUBO;

    void createScene(VulkanContext& inContext);

    void setGPUSceneData(const GPUSceneData& inData);

    void updateGPUBuffer();
};

#endif //ANVIL_VK_SCENE_H

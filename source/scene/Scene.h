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
struct GlobalSceneData
{
    glm::vec4 lightDirection;   /**< w = unused/padding */
    glm::vec4 lightColor;       /**< w = intensity */
    glm::vec4 ambientColor;     /**< w = unused/padding */

    uint32_t debugViewMode;     /**< Maps to the engine's DebugMode enum */
    glm::vec3 _padding;         /**< Strictly maintain 16-byte Vulkan UBO alignment */
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
    VulkanContext* pContext = nullptr;
    bool isDirty = true;

public:
    GlobalSceneData data{};
    GPUBuffer sceneUBO;

    /**
     * @brief Creates an empty scene with GPUBuffer that can be set and updated.
     * @param inContext Reference to the context, needed for buffer creation.
     */
    void createScene(VulkanContext& inContext);

    /**
     * @brief Sets the complete GPUSceneData at ones.
     * @param inData Object containing scene data.
     * @note Need to call updateGPUBuffer for changes to take effect.
     */
    void setGPUSceneData(const GlobalSceneData& inData);

    /**
     * @brief Updates the GPUBuffer with new SceneData.
     */
    void updateGPUBuffer();
};

#endif //ANVIL_VK_SCENE_H

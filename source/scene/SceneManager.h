// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SCENEMANAGER_H
#define ANVIL_VK_SCENEMANAGER_H

/**
 * @file SceneManager.h
 * @brief Orchestrates scene discovery, asset loading, and runtime scene switching.
 */

#include <vector>

#include "Camera.h"
#include "CPUModel.h"
#include "GPUModel.h"
#include "Scene.h"
#include "SceneConfig.h"

/**
 * @brief Manages the lifecycle of multiple scenes, handling disk loading and GPU uploads.
 *
 * Scans directories for scene configurations, tracks the currently active scene, and coordinates
 * the tear-down and stand-up of CPU/GPU models, cameras, and lighting data during scene transitions.
 */
class SceneManager
{
public:
    SceneManager() = default;
    ~SceneManager() = default;

    std::vector<SceneConfig> availableScenes;
    int activeSceneIndex = -1;

    CPUModel cpuModel;
    GPUModel gpuModel;

    /**
     * @brief Scans a directory for `.ini` scene configurations and populates availableScenes.
     * @param sceneDirectory The folder path to search for scene files.
     */
    void discoverScenes(const std::string& sceneDirectory);

    /**
     * @brief Tears down the current scene and loads a new one from the available configurations.
     *
     * Waits for the GPU to idle, resets the CPU and GPU models, applies the new camera and
     * lighting configurations, and triggers the required Vulkan uploads.
     *
     * @param sceneIndex The index of the target scene in the availableScenes vector.
     * @param inContext Reference to the active Vulkan context.
     * @param inMaterial The AnvilMaterial factory used to allocate descriptor sets for the new models.
     * @param camera Reference to the active camera to be updated with the new scene's starting position.
     * @param scene Reference to the active Scene object to be updated with new lighting data.
     * @return True if the scene was successfully loaded and deployed to the GPU; false otherwise.
     */
    bool loadScene(uint32_t sceneIndex, VulkanContext& inContext, const AnvilMaterial& inMaterial,
        Camera& camera, Scene& scene);

    /**
     * @brief Hot-reloads the currently active scene from disk.
     *
     * Useful for applying changes made to the `.ini` configuration file without restarting the engine.
     *
     * @param inContext Reference to the active Vulkan context.
     * @param inMaterial The AnvilMaterial factory used for descriptor sets.
     * @param camera Reference to the active camera.
     * @param scene Reference to the active Scene object.
     *
     * @throws std::runtime_error If called while activeSceneIndex is invalid.
     */
    void reloadActiveScene(VulkanContext& inContext, const AnvilMaterial& inMaterial,
        Camera& camera, Scene& scene);
};

#endif //ANVIL_VK_SCENEMANAGER_H

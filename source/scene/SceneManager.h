// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SCENEMANAGER_H
#define ANVIL_VK_SCENEMANAGER_H

#include <vector>

#include "Camera.h"
#include "CPUModel.h"
#include "GPUModel.h"
#include "Scene.h"
#include "SceneConfig.h"

class SceneManager
{
public:
    SceneManager() = default;
    ~SceneManager() = default;

    std::vector<SceneConfig> availableScenes;
    int activeSceneIndex = -1;

    CPUModel cpuModel;
    GPUModel gpuModel;

    void discoverScenes(const std::string& sceneDirectory);

    bool loadScene(uint32_t sceneIndex, VulkanContext& inContext, const AnvilMaterial& inMaterial,
        Camera& camera, Scene& scene);

    void reloadActiveScene(VulkanContext& inContext, const AnvilMaterial& inMaterial,
        Camera& camera, Scene& scene);
};

#endif //ANVIL_VK_SCENEMANAGER_H

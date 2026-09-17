// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "SceneManager.h"
#include "ScreenLogger.h"

void SceneManager::discoverScenes(const std::string& sceneDirectory)
{
    availableScenes.clear();

    if (!std::filesystem::exists(sceneDirectory))
    {
        std::cerr << "[SceneManager] Directory does not exist: " << sceneDirectory << std::endl;
        return;
    }

    // Checking for all ini files in the directory
    // Warning: will assume every ini file is scene config
    // All scene ini files will need to be placed in this one directory.
    // TODO: Improve the scene loading system
    for (const auto &entry : std::filesystem::directory_iterator(sceneDirectory))
    {
        if (entry.path().extension() == ".ini")
        {
            SceneConfig scene_config;
            if (SceneConfig::LoadFromFile(entry.path().string(), scene_config))
            {
                availableScenes.push_back(scene_config);
                std::cout << "[SceneManager] Discovered Scene: " << scene_config.sceneName << std::endl;
            }
        }
    }
}

bool SceneManager::loadScene(uint32_t sceneIndex, VulkanContext& inContext, const AnvilMaterial& inMaterial, Camera& camera, Scene& scene)
{
    if (sceneIndex >= availableScenes.size())
    {
        std::cerr << "[SceneManager] Scene does not exist with index: " << sceneIndex << std::endl;
        return false;
    }

    vkDeviceWaitIdle(inContext.device);

    SceneConfig& scene_config = availableScenes[sceneIndex];
    SceneConfig::LoadFromFile(scene_config.configPath, scene_config);

    try
    {
        // Tear down the old model and load the new model
        gpuModel.destroyGPUModel();
        cpuModel = CPUModel();
        cpuModel.loadGLTF(scene_config.modelPath);
        gpuModel.createGPUModel(inContext, cpuModel, inMaterial);

        // Reset camera
        camera = Camera();
        if (scene_config.cameraPosition)
            camera.position = *scene_config.cameraPosition;
        if (scene_config.cameraSpeed)
            camera.cameraSpeed = *scene_config.cameraSpeed;
        if (scene_config.cameraFovDegrees)
            camera.fovDegrees = *scene_config.cameraFovDegrees;

        // Reset lighting data
        GlobalSceneData light_data{};
        if (scene_config.lightDirection)
            light_data.lightDirection = *scene_config.lightDirection;
        if (scene_config.lightColor)
            light_data.lightColor = *scene_config.lightColor;
        if (scene_config.ambientColor)
            light_data.ambientColor = *scene_config.ambientColor;

        scene.setGPUSceneData(light_data);
        scene.updateGPUBuffer();

        activeSceneIndex = static_cast<int>(sceneIndex);

        LOGUI("[SceneManager] Loaded Scene: " + scene_config.sceneName, AnvilColor::Blue);
        return true;
    }
    catch (const std::exception& e)
    {
        LOGUI("[SceneManager] Failed to load scene: " + scene_config.sceneName, AnvilColor::Red);
        return false;
    }
}

void SceneManager::reloadActiveScene(VulkanContext& inContext, const AnvilMaterial& inMaterial, Camera& camera, Scene& scene)
{
    if (activeSceneIndex >= 0)
    {
        loadScene(static_cast<uint32_t>(activeSceneIndex), inContext, inMaterial, camera, scene);
    }
    else
    {
        throw std::runtime_error("[SceneManager::reloadActiveScene] Something went wrong. SceneIndex is bad.");
    }
}

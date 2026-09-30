// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "SceneManager.h"

#include "Logger.h"
#include "ScreenLogger.h"
#include "Trace.h"

void SceneManager::discoverScenes(const std::string& sceneDirectory)
{
    SCOPE_CPU;

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
                LOG_INFO("Discovered Scene: {}", scene_config.sceneName);
            }
        }
    }
}

bool SceneManager::loadScene(uint32_t sceneIndex, VulkanContext& inContext, const Material& inMaterial, Camera& camera, Scene& scene)
{
    SCOPE_CPU;

    if (sceneIndex >= availableScenes.size())
    {
        LOG_ERROR("Scene does not exist with index: {}", sceneIndex);
        return false;
    }

    vkDeviceWaitIdle(inContext.device);

    SceneConfig& scene_config = availableScenes[sceneIndex];
    if (!SceneConfig::LoadFromFile(scene_config.configPath, scene_config))
    {
        LOG_ERROR("Could not load scene config from file: {}", scene_config.configPath);
        return false;
    }

    // Tear down the old model and load the new model
    gpuModel.destroyGPUModel();
    cpuModel = CPUModel();

    // FIX: Prepend the absolute ASSETS_DIR macro so it ignores the Working Directory
    // TODO: Need to find a better way to do this
    std::string absoluteModelPath = std::string(ASSETS_DIR) + "/" + scene_config.modelPath;

    cpuModel.loadGLTF(absoluteModelPath);
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

    LOG_INFO("Loaded Scene: {}", scene_config.sceneName);
    LOGUI("[SceneManager] Loaded Scene: " + scene_config.sceneName, AnvilColor::Blue);
        return true;
}

void SceneManager::reloadActiveScene(VulkanContext& inContext, const Material& inMaterial, Camera& camera, Scene& scene)
{
    SCOPE_CPU;

    ENSURE(activeSceneIndex >= 0, "SceneIndex is bad.");
    loadScene(static_cast<uint32_t>(activeSceneIndex), inContext, inMaterial, camera, scene);
}

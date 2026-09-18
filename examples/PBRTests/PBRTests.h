// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_PBRTESTS_H
#define EXAMPLE_PBRTESTS_H

#include "AnvilMaterial.h"
#include "AnvilRenderer.h"
#include "Camera.h"
#include "GBuffer.h"
#include "Scene.h"
#include "SceneManager.h"
#include "ShaderCompiler.h"

class PBRTests
{
public:
    void initializeProject(VulkanContext& inContext, Swapchain& inSwapchain, AnvilRenderer& inRenderer);
    void cleanupProject();

    bool loadPipelines(std::string* outErrorMessage = nullptr);

    void recordGeometryPass(VkCommandBuffer inCmd, const Swapchain& inSwapchain);
    void recordLightingPass(VkCommandBuffer inCmd, Swapchain& inSwapchain);

private:
    bool loadGeometryPipeline(std::string* outErrorMessage);
    bool loadLightingPipeline(std::string* outErrorMessage);

    int pendingSceneIndex = -1;

    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;
    AnvilRenderer* pRenderer = nullptr;

    SceneManager sceneManager;
    Camera camera;
    Scene pbrScene;
    GBuffer gBuffer;

    ShaderCompiler shaderCompiler;

    ShaderProgram shaderProgram_Geo;
    AnvilMaterial material_Geo;
    AnvilPipeline pipeline_Geo;

    ShaderProgram shaderProgram_Light;
    AnvilMaterial material_Light;
    AnvilPipeline pipeline_Light;
    MaterialInstance sceneLightingSet;
};

#endif //EXAMPLE_PBRTESTS_H

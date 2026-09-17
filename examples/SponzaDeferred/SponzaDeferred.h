// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_SPONZADEFERRED_H
#define EXAMPLE_SPONZADEFERRED_H

#include "AnvilMaterial.h"
#include "AnvilRenderer.h"
#include "CPUModel.h"
#include "GPUModel.h"
#include "Scene.h"
#include "ShaderCompiler.h"
#include "PipelineBuilder.h"
#include "Swapchain.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "GBuffer.h"

class SponzaDeferred
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;
    AnvilRenderer* pRenderer = nullptr;

    ShaderCompiler shaderCompiler;

    CPUModel cpuModel;
    GPUModel gpuModel;

    GBuffer gBuffer;

    // Geometry Pass
    ShaderProgram shaderProgram_Geo;
    AnvilMaterial material_Geo;
    AnvilPipeline pipeline_Geo;

    // Lighting Pass
    ShaderProgram shaderProgram_Light;
    AnvilMaterial material_Light;
    AnvilPipeline pipeline_Light;

    MaterialInstance sceneLightingSet; // Manages set 0 : GlobalSceneData

    Scene sponzaScene;
    Camera camera;

public:
    void initializeProject(VulkanContext& inContext, Swapchain& inSwapchain, AnvilRenderer& inRenderer);
    void cleanupProject();
    bool loadPipelines(std::string* outErrorMessage = nullptr);

    void recordGeometryPass(VkCommandBuffer inCmd, const Swapchain& inSwapchain);
    void recordLightingPass(VkCommandBuffer inCmd, Swapchain& inSwapchain);

private:
    bool loadGeometryPipeline(std::string* outErrorMessage = nullptr);
    bool loadLightingPipeline(std::string* outErrorMessage = nullptr);
};


#endif //EXAMPLE_SPONZADEFERRED_H

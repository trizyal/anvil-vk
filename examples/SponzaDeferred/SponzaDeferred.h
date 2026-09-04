// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SPONZADEFERRED_H
#define ANVIL_VK_SPONZADEFERRED_H

#include "AnvilMaterial.h"
#include "CPUModel.h"
#include "GPUModel.h"
#include "Scene.h"
#include "ShaderCompiler.h"
#include "PipelineBuilder.h"
#include "Swapchain.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "GBuffer.h"

struct PushConstants
{
    glm::mat4 viewProjection;
    glm::vec4 camera;
    uint32_t objectIndex;
};

class SponzaDeferred
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;

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
    void initializeProject(VulkanContext& inContext, Swapchain& inSwapchain);
    void cleanupProject();
    bool loadPipelines(std::string* outErrorMessage = nullptr);

    void recordGeometryPass(VkCommandBuffer inCmd, const Swapchain& inSwapchain);
    void recordLightingPass(VkCommandBuffer inCmd, Swapchain& inSwapchain);

private:
    bool loadGeometryPipeline(std::string* outErrorMessage = nullptr);
    bool loadLightingPipeline(std::string* outErrorMessage = nullptr);
};


#endif //ANVIL_VK_SPONZADEFERRED_H

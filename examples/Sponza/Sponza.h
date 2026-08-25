// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_SPONZA_H
#define EXAMPLE_SPONZA_H

#include "AnvilMaterial.h"
#include "CPUModel.h"
#include "GPUModel.h"
#include "Scene.h"
#include "ShaderCompiler.h"
#include "PipelineBuilder.h"
#include "Swapchain.h"
#include "VulkanContext.h"
#include "Camera.h"

struct PushConstants
{
    glm::mat4 renderMatrix;
    glm::mat4 modelMatrix;
    glm::vec4 camera;
};

class Sponza
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;

    ShaderCompiler shaderCompiler;

    CPUModel cpuModel;
    GPUModel gpuModel;

    AnvilMaterial sponzaMaterial;
    MaterialInstance globalSet; // Manages Set 0
    Scene sponzaScene;
    Camera camera;

    AnvilPipeline pipeline;

public:
    void initializeProject(VulkanContext& inContext, Swapchain& inSwapchain);
    void cleanupProject();
    void loadPipeline();
    void recordCommands(VkCommandBuffer inCmd, Swapchain &inSwapchain);
};

#endif //EXAMPLE_SPONZA_H

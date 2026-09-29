// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_SPONZA_H
#define EXAMPLE_SPONZA_H

#include "Material.h"
#include "Renderer.h"
#include "CPUModel.h"
#include "GPUModel.h"
#include "Scene.h"
#include "ShaderCompiler.h"
#include "PipelineBuilder.h"
#include "Swapchain.h"
#include "VulkanContext.h"
#include "Camera.h"

class Sponza
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;
    Renderer* pRenderer = nullptr;

    ShaderCompiler shaderCompiler;

    CPUModel cpuModel;
    GPUModel gpuModel;

    ShaderProgram sponzaProgram;
    Material sponzaMaterial;
    MaterialInstance globalSet; // Manages Set 0
    Scene sponzaScene;
    Camera camera;

    AnvilPipeline pipeline;

public:
    void initializeProject(VulkanContext& inContext, Swapchain& inSwapchain, Renderer& inRenderer);
    void cleanupProject();
    bool loadPipeline(std::string* outErrorMessage = nullptr);
    void recordCommands(VkCommandBuffer inCmd, Swapchain &inSwapchain);
};

#endif //EXAMPLE_SPONZA_H

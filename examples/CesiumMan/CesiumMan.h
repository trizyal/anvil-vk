// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_CESIUMMAN_H
#define EXAMPLE_CESIUMMAN_H

#include "AnvilMaterial.h"
#include "CPUModel.h"
#include "GPUModel.h"
#include "Scene.h"
#include "ShaderCompiler.h"
#include "PipelineBuilder.h"
#include "Swapchain.h"
#include "VulkanContext.h"
#include "Camera.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix; /**< Projection * View * Model */
    glm::mat4 modelMatrix;  /**< Model rotation for world-space normals */
    glm::vec4 camera;
    glm::vec4 baseColorFactor;
};

class CesiumMan
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;

    ShaderCompiler shaderCompiler;

    CPUModel cpuModel;
    GPUModel gpuModel;

    AnvilMaterial cesiumMaterial;
    MaterialInstance globalSet;
    Scene cesiumScene;
    Camera camera;

    AnvilPipeline pipeline;

    float animationTime = 0.0f;

public:
    void initializeProject(VulkanContext& inContext, Swapchain& inSwapchain);
    void cleanupProject();
    void loadPipeline();
    void recordCommands(VkCommandBuffer inCmd, Swapchain &inSwapchain);
};

#endif //EXAMPLE_CESIUMMAN_H

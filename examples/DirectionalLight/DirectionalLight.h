// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_DIRECTIONALLIGHT_H
#define EXAMPLE_DIRECTIONALLIGHT_H

#include <glm/glm.hpp>

#include "Camera.h"
#include "AnvilMaterial.h"
#include "GPUMesh.h"
#include "GPUModel.h"
#include "VulkanContext.h"
#include "PipelineBuilder.h"
#include "Scene.h"
#include "ShaderCompiler.h"
#include "Swapchain.h"
#include "ShaderProgram.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct ProjectPushConstants
{
    glm::mat4 renderMatrix; /**< Projection * View * Model */
    glm::mat4 modelMatrix;  /**< Model rotation for world-space normals */
    glm::vec4 camera;
    glm::vec4 baseColorFactor;
};

class DirectionalLight
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;
    ShaderCompiler shaderCompiler;

    AnvilPipeline pipeline = {};
    Camera camera;
    Scene myScene;

    ShaderProgram myProgram; // Explicitly manage program layout
    AnvilMaterial myMaterial;
    MaterialInstance globalSet;

    CPUModel cpuModel;
    GPUModel gpuModel;

public:
    void initializeProject(VulkanContext& inAnvilContext, Swapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, Swapchain &inAnvilSwapchain);

    void loadPipeline();
};

#endif //EXAMPLE_DIRECTIONALLIGHT_H
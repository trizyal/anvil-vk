// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_BOXMODEL_H
#define EXAMPLE_BOXMODEL_H

#include <glm/glm.hpp>

#include "Camera.h"
#include "AnvilMaterial.h"
#include "CPUModel.h"
#include "GPUModel.h"
#include "VulkanContext.h"
#include "PipelineBuilder.h"
#include "ShaderCompiler.h"
#include "Swapchain.h"
#include "ShaderProgram.h" // Added explicit shader program

// The data we push to the shader every frame (Must be <= 128 bytes)
struct ProjectPushConstants
{
    glm::mat4 renderMatrix; /**< Projection * View * Model */
    glm::mat4 modelMatrix;  /**< Model rotation for world-space normals */
    glm::vec4 baseColorFactor;
};

class BoxModel
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;
    ShaderCompiler shaderCompiler;

    AnvilPipeline pipeline = {};
    Camera camera;

    ShaderProgram myProgram; // Explicitly manage program layout
    AnvilMaterial myMaterial;

    CPUModel cpuModel;
    GPUModel gpuModel;

public:
    void initializeProject(VulkanContext& inAnvilContext, Swapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, Swapchain &inAnvilSwapchain);

    void loadPipeline();
};

#endif //EXAMPLE_BOXMODEL_H
// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_SHADERREFLECTIONCUBE_H
#define EXAMPLE_SHADERREFLECTIONCUBE_H

#include <glm/glm.hpp>

#include "Camera.h"
#include "AnvilMaterial.h"
#include "CPUModel.h"
#include "GPUModel.h"
#include "VulkanContext.h"
#include "PipelineBuilder.h"
#include "ShaderCompiler.h"
#include "Swapchain.h"
#include "ShaderProgram.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct ProjectPushConstants
{
    glm::mat4 renderMatrix;
};

class ShaderReflectionCube
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

#endif //EXAMPLE_SHADERREFLECTIONCUBE_H

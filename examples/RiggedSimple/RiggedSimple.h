// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_RIGGEDSIMPLE_H
#define EXAMPLE_RIGGEDSIMPLE_H

#include <glm/glm.hpp>

#include "VulkanContext.h"
#include "Swapchain.h"
#include "ShaderCompiler.h"
#include "PipelineBuilder.h"
#include "Camera.h"
#include "Scene.h"
#include "AnvilMaterial.h"
#include "CPUModel.h"
#include "GPUModel.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix; /**< Projection * View * Model */
    glm::mat4 modelMatrix;  /**< Model rotation for world-space normals */
    glm::vec4 camera;
    glm::vec4 baseColorFactor;
};

class RiggedSimple
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;
    ShaderCompiler shaderCompiler;

    AnvilPipeline pipeline{};
    Camera camera;
    Scene riggedScene;

    AnvilMaterial riggedMaterial;

    CPUModel cpuModel;
    GPUModel gpuModel;

    float animationTime = 0.0f;

public:
    void initializeProject(VulkanContext& inContext, Swapchain& inSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, Swapchain &inSwapchain);

    void loadPipeline();
};


#endif //EXAMPLE_RIGGEDSIMPLE_H

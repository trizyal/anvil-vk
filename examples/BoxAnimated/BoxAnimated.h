// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_BOXANIMATED_H
#define EXAMPLE_BOXANIMATED_H

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

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix; /**< Projection * View * Model */
    glm::mat4 modelMatrix;  /**< Model rotation for world-space normals */
    glm::vec4 camera;
    glm::vec4 baseColorFactor;
};

class BoxAnimated
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;
    ShaderCompiler shaderCompiler;

    AnvilPipeline pipeline{};
    Camera camera;
    Scene boxScene;

    AnvilMaterial boxMaterial;

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


#endif //EXAMPLE_BOXANIMATED_H

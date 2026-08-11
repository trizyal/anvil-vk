// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_TRUCKMODEL_H
#define EXAMPLE_TRUCKMODEL_H

#include <glm/glm.hpp>

#include "Camera.h"
#include "AnvilMaterial.h"
#include "GPUMesh.h"
#include "VulkanContext.h"
#include "PipelineBuilder.h"
#include "Scene.h"
#include "ShaderCompiler.h"
#include "VulkanSwapchain.h"
#include "TextureLoader.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix; /**< Projection * View * Model */
    glm::mat4 modelMatrix;  /**< Model rotation for world-space normals */
    glm::vec3 camera;
};

class TruckModel
{
private:
    VulkanContext* pContext = nullptr;
    VulkanSwapchain* pSwapchain = nullptr;
    ShaderCompiler shaderCompiler;

    AnvilPipeline pipeline = {};
    Camera camera;
    Scene myScene;

    AnvilMaterial myMaterial;

    CPUModel models;
    std::vector<GPUMesh> meshBuffers;
    std::vector<AnvilTexture> textures;

public:
    void initializeProject(VulkanContext& inAnvilContext, VulkanSwapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, VulkanSwapchain &inAnvilSwapchain);

    void loadPipeline();
};

#endif //EXAMPLE_TRUCKMODEL_H

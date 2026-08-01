// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_DIRECTIONALLIGHT_H
#define EXAMPLE_DIRECTIONALLIGHT_H

#include <glm/glm.hpp>

#include "AnvilCamera.h"
#include "AnvilMaterial.h"
#include "AnvilMeshBuffer.h"
#include "AnvilVulkanContext.h"
#include "AnvilPipeline.h"
#include "AnvilShaderCompiler.h"
#include "AnvilSwapchain.h"
#include "AnvilTextureLoader.h"

struct SceneData
{
    glm::vec4 lightDirection;   /**< w = unused/padding */
    glm::vec4 lightColor;       /**< w = intensity */
    glm::vec4 ambientColor;     /**< w = unused/padding */
};

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix; /**< Projection * View * Model */
    glm::mat4 modelMatrix;  /**< Model rotation for world-space normals */
};

class DirectionalLight
{
private:
    AnvilVulkanContext* ptrAContext = nullptr;
    AnvilSwapchain* ptrASwapchain = nullptr;
    AnvilShaderCompiler shaderCompiler;

    AnvilPipeline pipeline = {};
    AnvilMeshBuffer meshBuffer;
    AnvilCamera camera;

    // Things for textures
    AnvilTexture myTexture;
    AnvilMaterial myMaterial;

    AnvilBuffer sceneUBO;
    SceneData sceneLighting{};

public:
    void initializeProject(AnvilVulkanContext& inAnvilContext, AnvilSwapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, AnvilSwapchain &inAnvilSwapchain);

    void loadPipeline();
};

#endif //EXAMPLE_DIRECTIONALLIGHT_H

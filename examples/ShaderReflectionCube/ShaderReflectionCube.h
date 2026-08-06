// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_SHADERREFLECTIONCUBE_H
#define EXAMPLE_SHADERREFLECTIONCUBE_H

#include <glm/glm.hpp>

#include "AnvilCamera.h"
#include "AnvilMaterial.h"
#include "AnvilMeshBuffer.h"
#include "VulkanContext.h"
#include "PipelineBuilder.h"
#include "AnvilShaderCompiler.h"
#include "VulkanSwapchain.h"
#include "AnvilTextureLoader.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix;
};

class ShaderReflectionCube
{
private:
    VulkanContext* ptrAContext = nullptr;
    VulkanSwapchain* ptrASwapchain = nullptr;
    AnvilShaderCompiler shaderCompiler;

    AnvilPipeline pipeline = {};
    AnvilMeshBuffer meshBuffer;
    AnvilCamera camera;

    // Things for textures
    AnvilTexture myTexture;
    AnvilMaterial myMaterial;

public:
    void initializeProject(VulkanContext& inAnvilContext, VulkanSwapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, VulkanSwapchain &inAnvilSwapchain);

    void loadPipeline();
};


#endif //EXAMPLE_SHADERREFLECTIONCUBE_H

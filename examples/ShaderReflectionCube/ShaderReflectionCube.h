// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_SHADERREFLECTIONCUBE_H
#define EXAMPLE_SHADERREFLECTIONCUBE_H

#include <glm/glm.hpp>

#include "Camera.h"
#include "AnvilMaterial.h"
#include "GPUMesh.h"
#include "VulkanContext.h"
#include "PipelineBuilder.h"
#include "ShaderCompiler.h"
#include "VulkanSwapchain.h"
#include "TextureLoader.h"

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
    ShaderCompiler shaderCompiler;

    AnvilPipeline pipeline = {};
    GPUMesh meshBuffer;
    Camera camera;

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

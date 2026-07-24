// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_TEXTURECUBE_H
#define EXAMPLE_TEXTURECUBE_H

#include <glm/glm.hpp>

#include "AnvilCamera.h"
#include "AnvilMeshBuffer.h"
#include "AnvilVulkanContext.h"
#include "AnvilShaderModule.h"
#include "AnvilPipeline.h"
#include "AnvilShaderCompiler.h"
#include "AnvilSwapchain.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix;
};

class TextureCube
{
private:
    AnvilVulkanContext* ptrAContext = nullptr;
    AnvilSwapchain* ptrASwapchain = nullptr;

    AnvilShaderModule vertexShader;
    AnvilShaderModule fragmentShader;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    AnvilPipeline pipeline = {};

    AnvilMeshBuffer meshBuffer;

    AnvilShaderCompiler shaderCompiler;

    AnvilCamera camera;

public:
    void initializeProject(AnvilVulkanContext& inAnvilContext, AnvilSwapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, AnvilSwapchain &inAnvilSwapchain);

    void loadPipeline();
};


#endif //EXAMPLE_TEXTURECUBE_H

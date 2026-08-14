// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_BOXMODEL_H
#define EXAMPLE_BOXMODEL_H

#include <glm/glm.hpp>

#include "GPUBuffer.h"
#include "Camera.h"
#include "GPUMesh.h"
#include "VulkanContext.h"
#include "ShaderModule.h"
#include "PipelineBuilder.h"
#include "ShaderCompiler.h"
#include "Swapchain.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix;
};

class BoxModel
{
private:
    VulkanContext* ptrAContext = nullptr;
    Swapchain* ptrASwapchain = nullptr;

    ShaderModule vertexShader;
    ShaderModule fragmentShader;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    AnvilPipeline pipeline = {};

    GPUMesh meshBuffer;

    ShaderCompiler shaderCompiler;

    Camera camera;

public:
    void initializeProject(VulkanContext& inAnvilContext, Swapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, Swapchain &inAnvilSwapchain);

    void loadPipeline();
};


#endif //EXAMPLE_BOXMODEL_H

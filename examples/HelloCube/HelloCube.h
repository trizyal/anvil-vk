// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_HELLOCUBE_H
#define EXAMPLE_HELLOCUBE_H

#include <glm/glm.hpp>

#include "GPUBuffer.h"
#include "VulkanContext.h"
#include "ShaderModule.h"
#include "PipelineBuilder.h"
#include "AnvilShaderCompiler.h"
#include "VulkanSwapchain.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix;
};

class HelloCube
{
private:
    VulkanContext* ptrAContext = nullptr;
    VulkanSwapchain* ptrASwapchain = nullptr;

    ShaderModule vertexShader;
    ShaderModule fragmentShader;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    AnvilPipeline pipeline = {};

    GPUBuffer vertexBuffer;
    GPUBuffer indexBuffer;

    AnvilShaderCompiler shaderCompiler;

public:
    void initalizeProject(VulkanContext& inAnvilContext, VulkanSwapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, VulkanSwapchain &inAnvilSwapchain);

    void loadPipeline();

private:
    void createBuffers();
};


#endif //EXAMPLE_HELLOCUBE_H

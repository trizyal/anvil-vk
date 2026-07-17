// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_HELLOCUBE_H
#define EXAMPLE_HELLOCUBE_H

#include "AnvilBuffer.h"
#include "AnvilVulkanContext.h"
#include "AnvilShaderModule.h"
#include "AnvilPipeline.h"
#include "AnvilShaderCompiler.h"
#include "AnvilSwapchain.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    // glm::mat4 renderMatrix;
};

class HelloCube
{
private:
    AnvilVulkanContext* ptrAContext = nullptr;
    AnvilSwapchain* ptrASwapchain = nullptr;

    AnvilShaderModule vertexShader;
    AnvilShaderModule fragmentShader;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    AnvilPipeline pipeline = {};

    AnvilBuffer vertexBuffer;
    AnvilBuffer indexBuffer;

    AnvilShaderCompiler shaderCompiler;

public:
    void initalizeProject(AnvilVulkanContext& inAnvilContext, AnvilSwapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, AnvilSwapchain &inAnvilSwapchain);

    void loadPipeline();

private:
    void createBuffers();
};


#endif //EXAMPLE_HELLOCUBE_H

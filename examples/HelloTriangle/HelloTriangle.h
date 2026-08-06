// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_HELLOTRIANGLE_H
#define EXAMPLE_HELLOTRIANGLE_H

#include "AnvilVulkanContext.h"
#include "ShaderModule.h"
#include "PipelineBuilder.h"
#include "AnvilShaderCompiler.h"
#include "AnvilSwapchain.h"

class HelloTriangle
{
private:
    AnvilVulkanContext* ptrAContext = nullptr;
    AnvilSwapchain* ptrASwapchain = nullptr;

    ShaderModule vertexShader;
    ShaderModule fragmentShader;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    AnvilPipeline pipeline = {};

    AnvilShaderCompiler shaderCompiler;

public:
    void initalizeProject(AnvilVulkanContext& inAnvilContext, AnvilSwapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, AnvilSwapchain &inAnvilSwapchain);

    void loadPipeline();
};

#endif //EXAMPLE_HELLOTRIANGLE_H

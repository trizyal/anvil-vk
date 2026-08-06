// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_HELLOTRIANGLE_H
#define EXAMPLE_HELLOTRIANGLE_H

#include "VulkanContext.h"
#include "ShaderModule.h"
#include "PipelineBuilder.h"
#include "ShaderCompiler.h"
#include "VulkanSwapchain.h"

class HelloTriangle
{
private:
    VulkanContext* ptrAContext = nullptr;
    VulkanSwapchain* ptrASwapchain = nullptr;

    ShaderModule vertexShader;
    ShaderModule fragmentShader;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    AnvilPipeline pipeline = {};

    ShaderCompiler shaderCompiler;

public:
    void initalizeProject(VulkanContext& inAnvilContext, VulkanSwapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, VulkanSwapchain &inAnvilSwapchain);

    void loadPipeline();
};

#endif //EXAMPLE_HELLOTRIANGLE_H

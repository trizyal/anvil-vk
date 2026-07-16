// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_HELLOTRIANGLE_H
#define EXAMPLE_HELLOTRIANGLE_H

#include "AnvilVulkanContext.h"
#include "AnvilShaderModule.h"
#include "AnvilPipeline.h"
#include "AnvilSwapchain.h"

class HelloTriangle
{
private:
    AnvilVulkanContext* ptrAContext = nullptr;
    AnvilSwapchain* ptrASwapchain = nullptr;

    AnvilShaderModule vertexShader;
    AnvilShaderModule fragmentShader;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    AnvilPipeline pipeline = {};

public:
    void initalizeProject(AnvilVulkanContext* inAnvilContext, AnvilSwapchain* inAnvilSwapchain);
    void cleanup();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, AnvilSwapchain &inAnvilSwapchain);
};

#endif //EXAMPLE_HELLOTRIANGLE_H

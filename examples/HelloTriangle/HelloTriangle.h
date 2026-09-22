// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_HELLOTRIANGLE_H
#define EXAMPLE_HELLOTRIANGLE_H

#include "VulkanContext.h"
#include "AnvilMaterial.h"
#include "PipelineBuilder.h"
#include "ShaderCompiler.h"
#include "Swapchain.h"
#include "ShaderProgram.h"

class HelloTriangle
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;

    ShaderCompiler shaderCompiler;

    ShaderProgram myProgram; // Explicitly manage program layout
    AnvilMaterial myMaterial;
    AnvilPipeline pipeline = {};

public:
    void initializeProject(VulkanContext& inAnvilContext, Swapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, Swapchain &inAnvilSwapchain);

    void loadPipeline();
};

#endif //EXAMPLE_HELLOTRIANGLE_H
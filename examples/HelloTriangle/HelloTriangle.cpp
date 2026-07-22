// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "HelloTriangle.h"

#include <iostream>
#include <stdexcept>

#include "AnvilShaderCompiler.h"

void HelloTriangle::initalizeProject(AnvilVulkanContext& inAnvilContext, AnvilSwapchain& inAnvilSwapchain)
{
    ptrAContext = &inAnvilContext;
    ptrASwapchain = &inAnvilSwapchain;

    // Initialize shader compiler
    // AnvilShaderCompiler shaderCompiler;
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);

    loadPipeline();
}

void HelloTriangle::cleanupProject()
{
    if (ptrAContext)
    {
        vkDestroyPipelineLayout(ptrAContext->anvilDevice, pipelineLayout, nullptr);
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
        vertexShader.destroyShaderModule();
        fragmentShader.destroyShaderModule();
    }
}

void HelloTriangle::recordCommands(VkCommandBuffer inCmd, AnvilSwapchain &inAnvilSwapchain)
{
    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    // Set Dynamic States required by your AnvilPipelineBuilder
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(inAnvilSwapchain.anvilExtent.width);
    viewport.height = static_cast<float>(inAnvilSwapchain.anvilExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(inCmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = inAnvilSwapchain.anvilExtent;
    vkCmdSetScissor(inCmd, 0, 1, &scissor);

    // Draw
    vkCmdDraw(inCmd, 3, 1, 0, 0);
}

void HelloTriangle::loadPipeline()
{
    std::cout << "Creating HelloTriangle pipeline." << std::endl;
    // NO wait idle here. Anvil handled it.
    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
        vkDestroyPipelineLayout(ptrAContext->anvilDevice, pipelineLayout, nullptr);
    }
    vertexShader.destroyShaderModule();
    fragmentShader.destroyShaderModule();

    // Create shader compilation request
    AnvilShaders::ShaderCompileRequest vReq{"HelloTriangle", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"HelloTriangle", "fragmentMain", AnvilShaders::ST_Fragment};

    // Compile shaders
    auto vSpirv = shaderCompiler.compileToSPIRV(vReq);
    auto fSpirv = shaderCompiler.compileToSPIRV(fReq);

    // Create shader modules
    if (!vertexShader.createShaderModule(ptrAContext->anvilDevice, vSpirv))
    {
        throw std::runtime_error("Failed to create vertex shader module!");
    }

    if (!fragmentShader.createShaderModule(ptrAContext->anvilDevice, fSpirv))
    {
        throw std::runtime_error("Failed to create fragment shader module!");
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(ptrAContext->anvilDevice, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // Create pipeline
    AnvilPipelineBuilder pipelineBuilder;

    pipeline = pipelineBuilder.setShaders(vertexShader.get(), fragmentShader.get())
        .setColorAttachmentFormat(ptrASwapchain->anvilImageFormat)
        .setDepthAttachmentFormat(ptrASwapchain->depthFormat)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
        .disableBlending()
        .buildPipeline(ptrAContext->anvilDevice, pipelineLayout, "HelloTrianglePipeline");
}

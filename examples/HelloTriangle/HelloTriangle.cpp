// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "HelloTriangle.h"

#include <iostream>
#include <stdexcept>

#include "ShaderCompiler.h"
#include "DebugNames.h"

void HelloTriangle::initializeProject(VulkanContext& inAnvilContext, Swapchain& inAnvilSwapchain)
{
    pContext = &inAnvilContext;
    pSwapchain = &inAnvilSwapchain;

    // Initialize shader compiler
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);

    loadPipeline();
}

void HelloTriangle::cleanupProject()
{
    if (pContext)
    {
        vkDeviceWaitIdle(pContext->device);

        myMaterial.destroyMaterial();
        myProgram.destroyProgram(); // Clean up explicit program

        if (pipeline.pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
            pipeline.pipeline = VK_NULL_HANDLE;
        }

        shaderCompiler.shutdownShaderCompiler();
    }
}

void HelloTriangle::recordCommands(VkCommandBuffer inCmd, Swapchain &inAnvilSwapchain)
{
    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    // Set Dynamic States required by your AnvilPipelineBuilder
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(inAnvilSwapchain.swapchainExtent.width);
    viewport.height = static_cast<float>(inAnvilSwapchain.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(inCmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = inAnvilSwapchain.swapchainExtent;
    vkCmdSetScissor(inCmd, 0, 1, &scissor);

    // Draw the hardcoded 3 vertices
    vkCmdDraw(inCmd, 3, 1, 0, 0);
}

void HelloTriangle::loadPipeline()
{
    std::cout << "Creating HelloTriangle pipeline." << std::endl;

    shaderCompiler.resetSession();

    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
        pipeline.pipeline = VK_NULL_HANDLE;
        myMaterial.destroyMaterial();
        myProgram.destroyProgram(); // Clean up explicit program
    }

    // Create shader compilation request
    AnvilShaders::ShaderCompileRequest vReq{"HelloTriangle", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"HelloTriangle", "fragmentMain", AnvilShaders::ST_Fragment};

    // Split build process to build Program then Material
    myProgram.buildProgram(*pContext, shaderCompiler, vReq, fReq);
    myMaterial.buildMaterialFromProgram(*pContext, myProgram);

    // Create pipeline (Passing empty vectors for vertex input because it's hardcoded in shader)
    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(myMaterial.getVertexShader(), myMaterial.getFragmentShader())
        .setVertexInput({}, {})
        .setColorAttachmentFormats({pSwapchain->swapchainFormat})
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(false, VK_COMPARE_OP_ALWAYS) // No depth testing needed for a single triangle
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
        .disableBlending()
        .buildPipeline(pContext->device, myMaterial.materialPipelineLayout DNAME("HelloTrianglePipeline"));
}
// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "HelloCube.h"

#include <iostream>
#include <stdexcept>

#include "AnvilShaderCompiler.h"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
};

// 8 corners of a cube
const std::vector<Vertex> cubeVertices = {
    {{-1, -1, -1}, {1, 0, 0}}, {{ 1, -1, -1}, {0, 1, 0}},
    {{ 1,  1, -1}, {0, 0, 1}}, {{-1,  1, -1}, {1, 1, 0}},
    {{-1, -1,  1}, {1, 0, 1}}, {{ 1, -1,  1}, {0, 1, 1}},
    {{ 1,  1,  1}, {1, 1, 1}}, {{-1,  1,  1}, {0, 0, 0}}
};

// 36 indices for 12 triangles
const std::vector<uint16_t> cubeIndices = {
    0,1,2, 2,3,0, 1,5,6, 6,2,1, 7,6,5, 5,4,7,
    4,0,3, 3,7,4, 4,5,1, 1,0,4, 3,2,6, 6,7,3
};

void HelloCube::initalizeProject(AnvilVulkanContext& inAnvilContext, AnvilSwapchain& inAnvilSwapchain)
{
    ptrAContext = &inAnvilContext;
    ptrASwapchain = &inAnvilSwapchain;

    createBuffers();

    // Initialize shader compiler
    // AnvilShaderCompiler shaderCompiler;
    if (!shaderCompiler.init())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    loadPipeline();
}

void HelloCube::cleanupProject()
{
    if (ptrAContext)
    {
        vkDestroyPipelineLayout(ptrAContext->anvilDevice, pipelineLayout, nullptr);
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
        vertexShader.destroy();
        fragmentShader.destroy();
    }
}

void HelloCube::recordCommands(VkCommandBuffer inCmd, AnvilSwapchain &inAnvilSwapchain)
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

void HelloCube::loadPipeline()
{
    std::cout << "Creating HelloCube pipeline." << std::endl;
    // NO wait idle here. Anvil handled it.
    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
        vkDestroyPipelineLayout(ptrAContext->anvilDevice, pipelineLayout, nullptr);
    }
    vertexShader.destroy();
    fragmentShader.destroy();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(ptrAContext->anvilDevice, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // Create shader compilation request
    AnvilShaders::ShaderCompileRequest vReq{"HelloCube", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"HelloCube", "fragmentMain", AnvilShaders::ST_Fragment};

    // Compile shaders
    auto vSpirv = shaderCompiler.compileToSPIRV(vReq);
    auto fSpirv = shaderCompiler.compileToSPIRV(fReq);

    // Create shader modules
    if (!vertexShader.create(ptrAContext->anvilDevice, vSpirv))
    {
        throw std::runtime_error("Failed to create vertex shader module!");
    }

    if (!fragmentShader.create(ptrAContext->anvilDevice, fSpirv))
    {
        throw std::runtime_error("Failed to create fragment shader module!");
    }

    // Vertex Descriptions
    std::vector<VkVertexInputBindingDescription> bindings = {
        {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    std::vector<VkVertexInputAttributeDescription> attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}
    };

    // Create pipeline
    AnvilPipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(vertexShader.get(), fragmentShader.get())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormat(ptrASwapchain->anvilImageFormat)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
        .disableBlending()
        .build(ptrAContext->anvilDevice, pipelineLayout);
}

void HelloCube::createBuffers()
{
    vertexBuffer.createAndUpload(
        ptrAContext->anvilAllocator,
        cubeVertices.data(),
        cubeVertices.size() * sizeof(Vertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );

    indexBuffer.createAndUpload(
        ptrAContext->anvilAllocator,
        cubeIndices.data(),
        cubeIndices.size() * sizeof(uint16_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );
}

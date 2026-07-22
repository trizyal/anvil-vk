// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "HelloCube.h"

#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "AnvilShaderCompiler.h"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
};

#if 0
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
#endif

// 24 vertices (4 per face) to prevent color interpolation
const std::vector<Vertex> cubeVertices = {
    // Front face (Z = -1) - Red
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}}, // 0
    {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}}, // 1
    {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}}, // 2
    {{-1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}}, // 3

    // Right face (X = 1) - Green
    {{ 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}}, // 4
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}}, // 5
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}}, // 6
    {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}}, // 7

    // Back face (Z = 1) - Blue
    {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}}, // 8
    {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}}, // 9
    {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}}, // 10
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}}, // 11

    // Left face (X = -1) - Yellow
    {{-1.0f, -1.0f,  1.0f}, {1.0f, 1.0f, 0.0f}}, // 12
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}}, // 13
    {{-1.0f,  1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}}, // 14
    {{-1.0f,  1.0f,  1.0f}, {1.0f, 1.0f, 0.0f}}, // 15

    // Top face (Y = 1) - Cyan
    {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}}, // 16
    {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 1.0f}}, // 17
    {{ 1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 1.0f}}, // 18
    {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 1.0f}}, // 19

    // Bottom face (Y = -1) - Magenta
    {{-1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 1.0f}}, // 20
    {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 1.0f}}, // 21
    {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}}, // 22
    {{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 1.0f}}  // 23
};

// 36 indices for 12 triangles (2 triangles per face)
const std::vector<uint16_t> cubeIndices = {
    // Front
    0, 1, 2, 2, 3, 0,
    // Right
    4, 5, 6, 6, 7, 4,
    // Back
    8, 9, 10, 10, 11, 8,
    // Left
    12, 13, 14, 14, 15, 12,
    // Top
    16, 17, 18, 18, 19, 16,
    // Bottom
    20, 21, 22, 22, 23, 20
};

void HelloCube::initalizeProject(AnvilVulkanContext& inAnvilContext, AnvilSwapchain& inAnvilSwapchain)
{
    ptrAContext = &inAnvilContext;
    ptrASwapchain = &inAnvilSwapchain;

    createBuffers();

    // Initialize shader compiler
    // AnvilShaderCompiler shaderCompiler;
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);

    loadPipeline();
}

void HelloCube::cleanupProject()
{
    if (ptrAContext)
    {
        vertexBuffer.destroyBuffer(ptrAContext->anvilAllocator);
        indexBuffer.destroyBuffer(ptrAContext->anvilAllocator);
        vkDestroyPipelineLayout(ptrAContext->anvilDevice, pipelineLayout, nullptr);
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
        vertexShader.destroyShaderModule();
        fragmentShader.destroyShaderModule();
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

    // Calculate C++ Transforms
    static float time = 0.0f;
    time += 0.016f; // Simple delta time

    float aspect = static_cast<float>(inAnvilSwapchain.anvilExtent.width) / static_cast<float>(inAnvilSwapchain.anvilExtent.height);

    glm::mat4 projection = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 100.0f);
    projection[1][1] *= -1; // Flip Y for Vulkan

    glm::mat4 view = glm::lookAt(glm::vec3(0, 2, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), time, glm::vec3(0.5f, 1.0f, 0.0f));

    PushConstants constants{};
    constants.renderMatrix = projection * view * model;
    vkCmdPushConstants(inCmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &constants);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(inCmd, 0, 1, &vertexBuffer.buffer, &offset);
    vkCmdBindIndexBuffer(inCmd, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

    // Draw
    vkCmdDrawIndexed(inCmd, 36, 1, 0, 0, 0);
}

void HelloCube::loadPipeline()
{
    std::cout << "Creating HelloCube pipeline." << std::endl;
    // NO wait idle here. Anvil handled it.
    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
        vkDestroyPipelineLayout(ptrAContext->anvilDevice, pipelineLayout, nullptr);
    }
    vertexShader.destroyShaderModule();
    fragmentShader.destroyShaderModule();

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
    if (!vertexShader.createShaderModule(ptrAContext->anvilDevice, vSpirv))
    {
        throw std::runtime_error("Failed to create vertex shader module!");
    }

    if (!fragmentShader.createShaderModule(ptrAContext->anvilDevice, fSpirv))
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
        .setDepthAttachmentFormat(ptrASwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
        .disableBlending()
        .buildPipeline(ptrAContext->anvilDevice, pipelineLayout);
}

void HelloCube::createBuffers()
{
    vertexBuffer.createBuffer(
        ptrAContext->anvilAllocator,
        ptrAContext->anvilDevice,
        cubeVertices.data(),
        cubeVertices.size() * sizeof(Vertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        "CubeVertexBuffer"
    );

    indexBuffer.createBuffer(
        ptrAContext->anvilAllocator,
        ptrAContext->anvilDevice,
        cubeIndices.data(),
        cubeIndices.size() * sizeof(uint16_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        "CubeIndexBuffer"
    );
}

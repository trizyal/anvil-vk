// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "HelloCube.h"

#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "AnvilRenderer.h"
#include "UIElements.h"

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

void HelloCube::initializeProject(VulkanContext& inAnvilContext, Swapchain& inAnvilSwapchain)
{
    pContext = &inAnvilContext;
    pSwapchain = &inAnvilSwapchain;

    createBuffers();

    // Setup initial scene values to interact with the shader
    GlobalSceneData sceneLighting{};
    sceneLighting.lightDirection = glm::vec4(-1.0f, -1.0f, -0.5f, 0.0f);
    sceneLighting.lightColor = glm::vec4(1.0f, 0.95f, 0.8f, 2.0f); // Warm sunlight to tint the cube
    sceneLighting.ambientColor = glm::vec4(0.08f, 0.1f, 0.15f, 1.0f);

    myScene.createScene(*pContext);
    myScene.setGPUSceneData(sceneLighting);
    myScene.updateGPUBuffer();

    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);
    loadPipeline();
}

void HelloCube::cleanupProject()
{
    if (pContext)
    {
        vkDeviceWaitIdle(pContext->device);

        vertexBuffer.destroyBuffer();
        indexBuffer.destroyBuffer();

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

void HelloCube::createBuffers()
{
    vertexBuffer.createBuffer(
        *pContext,
        cubeVertices.data(),
        cubeVertices.size() * sizeof(Vertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        DNAME("CubeVertexBuffer")
    );

    indexBuffer.createBuffer(
        *pContext,
        cubeIndices.data(),
        cubeIndices.size() * sizeof(uint16_t), // Important! Must map to UINT16 draw calls
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        DNAME("CubeIndexBuffer")
    );
}

void HelloCube::loadPipeline()
{
    std::cout << "Creating HelloCube pipeline." << std::endl;

    shaderCompiler.resetSession();

    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
        pipeline.pipeline = VK_NULL_HANDLE;
        myMaterial.destroyMaterial();
        myProgram.destroyProgram();
    }

    AnvilShaders::ShaderCompileRequest vReq{"HelloCube", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"HelloCube", "fragmentMain", AnvilShaders::ST_Fragment};

    // Split build process to build Program then Material
    myProgram.buildProgram(*pContext, shaderCompiler, vReq, fReq);
    myMaterial.buildMaterialFromProgram(*pContext, myProgram);

    // Allocate and update Set 0 (Global Scene UBO)
    globalSet = myMaterial.allocateSet(0);
    globalSet.bindUniformBuffer("sceneBuffer", myScene.sceneUBO);
    globalSet.updateDescriptorSets();

    // Define custom vertex inputs manually since we aren't using GPUMesh/MeshVertex here!
    std::vector<VkVertexInputBindingDescription> bindings = {
        {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
    };
    std::vector<VkVertexInputAttributeDescription> attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}
    };

    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(myMaterial.getVertexShader(), myMaterial.getFragmentShader())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormats({pSwapchain->swapchainFormat})
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE) // Note: Hardcoded cube indices often require CLOCKWISE
        .disableBlending()
        .buildPipeline(pContext->device, myMaterial.materialPipelineLayout DNAME("HelloCubePipeline"));
}

void HelloCube::recordCommands(VkCommandBuffer inCmd, Swapchain &inAnvilSwapchain)
{
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

    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    static float time = 0.0f;
    time += deltaTime;

    camera.updateCamera(deltaTime);

    float aspect = static_cast<float>(inAnvilSwapchain.swapchainExtent.width) / static_cast<float>(inAnvilSwapchain.swapchainExtent.height);

    glm::mat4 projection = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), time, glm::vec3(0.5f, 1.0f, 0.0f));

    UI::RenderWorldAxes(view);

    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    if (globalSet.descriptorSet != VK_NULL_HANDLE)
    {
        vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, myMaterial.materialPipelineLayout,
            0, 1, &globalSet.descriptorSet, 0, nullptr);
    }

    ProjectPushConstants constants{};
    constants.renderMatrix = projection * view * model;

    vkCmdPushConstants(inCmd, myMaterial.materialPipelineLayout, myMaterial.pushConstantStages, 0,
        sizeof(ProjectPushConstants), &constants);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(inCmd, 0, 1, &vertexBuffer.buffer, &offset);

    // Hardcoded cubes use UINT16 indices!
    vkCmdBindIndexBuffer(inCmd, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(inCmd, static_cast<uint32_t>(cubeIndices.size()), 1, 0, 0, 0);

    AnvilRenderer::engineStats.drawCalls++;
    AnvilRenderer::engineStats.primitiveCount += (static_cast<uint32_t>(cubeIndices.size()) / 3);
}
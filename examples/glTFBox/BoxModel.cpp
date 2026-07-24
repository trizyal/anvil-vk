// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "BoxModel.h"

#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "AnvilMeshBuffer.h"
#include "AnvilMeshLoader.h"
#include "AnvilShaderCompiler.h"
#include "AnvilUIRenderer.h"

void BoxModel::initializeProject(AnvilVulkanContext& inAnvilContext, AnvilSwapchain& inAnvilSwapchain)
{
    ptrAContext = &inAnvilContext;
    ptrASwapchain = &inAnvilSwapchain;

    const char* modelPath = PROJECT_DIR "/Box/glTF/Box.gltf";
    // const char* modelPath = PROJECT_DIR "/BoxVertexColors/glTF/BoxVertexColors.gltf";
    // const char* modelPath = PROJECT_DIR "/BoxInterleaved/glTF/BoxInterleaved.gltf";
    const AnvilMesh cubeMesh = AnvilModelLoader::LoadGLTF(modelPath);

    meshBuffer.createAnvilMeshBuffer(*ptrAContext, cubeMesh);

    // Initialize shader compiler
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);
    loadPipeline();
}

void BoxModel::cleanupProject()
{
    if (ptrAContext)
    {
        meshBuffer.destroyAnvilMeshBuffer(*ptrAContext);
        vkDestroyPipelineLayout(ptrAContext->anvilDevice, pipelineLayout, nullptr);
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
        vertexShader.destroyShaderModule();
        fragmentShader.destroyShaderModule();
    }
}

void BoxModel::recordCommands(VkCommandBuffer inCmd, AnvilSwapchain &inAnvilSwapchain)
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
    static float dt = 0.016f; // Simple delta time

    camera.updateCamera(dt);

    float aspect = static_cast<float>(inAnvilSwapchain.anvilExtent.width) / static_cast<float>(inAnvilSwapchain.anvilExtent.height);

    glm::mat4 projection = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();

    AnvilUIRenderer::DrawDebugAxis(view);

    PushConstants constants;
    constants.renderMatrix = projection * view;
    vkCmdPushConstants(inCmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &constants);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(inCmd, 0, 1, &meshBuffer.vertexBuffer.buffer, &offset);

    // This was VK_INDEX_TYPE_UINT16, but everything else uses 32
    // Caused a bug where half the triangles were not being rendered.
    vkCmdBindIndexBuffer(inCmd, meshBuffer.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw
    vkCmdDrawIndexed(inCmd, meshBuffer.indexCount, 1, 0, 0, 0);
}

void BoxModel::loadPipeline()
{
    std::cout << "Creating BoxModel pipeline." << std::endl;
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
    AnvilShaders::ShaderCompileRequest vReq{"BoxModel", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"BoxModel", "fragmentMain", AnvilShaders::ST_Fragment};

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

    auto something = AnvilMeshBuffer::getAttributeDescriptions();

    // Vertex Descriptions
    std::vector<VkVertexInputBindingDescription> bindings = {AnvilMeshBuffer::getBindingDescription()};
    std::vector<VkVertexInputAttributeDescription> attributes =
        {something[0], something[1]};

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


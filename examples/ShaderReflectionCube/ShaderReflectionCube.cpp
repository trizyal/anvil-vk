// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "ShaderReflectionCube.h"

#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "GPUMesh.h"
#include "CPUModel.h"
#include "ShaderCompiler.h"
#include "TextureLoader.h"
#include "UIElements.h"
#include "UIRenderer.h"

void ShaderReflectionCube::initializeProject(VulkanContext& inAnvilContext, Swapchain& inAnvilSwapchain)
{
    ptrAContext = &inAnvilContext;
    ptrASwapchain = &inAnvilSwapchain;

    const char* modelPath = PROJECT_DIR "/Cube/glTF/Cube.gltf";
    const CPUMesh_Single cubeMesh = ModelLoader::LoadSingleMeshGLTF(modelPath);
    meshBuffer.createGPUMesh(*ptrAContext, cubeMesh);

    if (!cubeMesh.texturePath.empty())
    {
        std::cout << "Loading texture: " << cubeMesh.texturePath << std::endl;

        myTexture = TextureLoader::LoadTexture(
            cubeMesh.texturePath,
            *ptrAContext
        );
    }

    // Initialize shader compiler
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);
    loadPipeline();
}

void ShaderReflectionCube::cleanupProject()
{
    if (ptrAContext)
    {
        myTexture.destroyAnvilTexture(ptrAContext);
        myMaterial.destroyMaterial();
        meshBuffer.destroyGPUMesh();
        vkDestroyPipeline(ptrAContext->device, pipeline.pipeline, nullptr);
    }
}

void ShaderReflectionCube::recordCommands(VkCommandBuffer inCmd, Swapchain &inAnvilSwapchain)
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

    // Calculate C++ Transforms
    static float time = 0.0f;
    static float dt = 0.016f; // Simple delta time

    camera.updateCamera(dt);

    const float aspect = static_cast<float>(inAnvilSwapchain.swapchainExtent.width) / static_cast<float>(inAnvilSwapchain.swapchainExtent.height);

    const glm::mat4 projection = camera.getProjectionMatrix(aspect);
    const glm::mat4 view = camera.getViewMatrix();

    UI::RenderWorldAxes(view);

    PushConstants constants{};
    constants.renderMatrix = projection * view;
    vkCmdPushConstants(inCmd, myMaterial.materialPipelineLayout, myMaterial.pushConstantStages, 0, sizeof(PushConstants), &constants);

    vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, myMaterial.materialPipelineLayout, 0, 1, &myMaterialInstance.descriptorSet, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(inCmd, 0, 1, &meshBuffer.vertexBuffer.buffer, &offset);

    // This was VK_INDEX_TYPE_UINT16, but everything else uses 32
    // Caused a bug where half the triangles were not being rendered.
    vkCmdBindIndexBuffer(inCmd, meshBuffer.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw
    vkCmdDrawIndexed(inCmd, meshBuffer.indexCount, 1, 0, 0, 0);
}

void ShaderReflectionCube::loadPipeline()
{
    std::cout << "Creating ShaderReflectionCube pipeline." << std::endl;

    shaderCompiler.resetSession();

    // NO wait idle here. Anvil handled it.
    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ptrAContext->device, pipeline.pipeline, nullptr);
        myMaterial.destroyMaterial();
    }

    // Create shader compilation request
    AnvilShaders::ShaderCompileRequest vReq{"ShaderReflectionCube", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"ShaderReflectionCube", "fragmentMain", AnvilShaders::ST_Fragment};

    // One call for material: Compile, Reflect, Shader Modules, and Build Layouts
    myMaterial.buildMaterial(*ptrAContext, shaderCompiler, vReq, fReq);

    // Bind by name
    if (myTexture.imageView != VK_NULL_HANDLE)
    {
        myMaterialInstance = myMaterial.createInstance(); // Spawn it!
        myMaterialInstance.bindTexture("texSampler", myTexture);
        myMaterialInstance.updateDescriptorSets();
    }

    auto attributesArray = GPUMesh::GetAttributeDescriptionsArray3();

    // Vertex Descriptions
    std::vector<VkVertexInputBindingDescription> bindings = {GPUMesh::GetBindingDescription()};
    std::vector<VkVertexInputAttributeDescription> attributes =
        {attributesArray[0], attributesArray[1], attributesArray[2]};

    // Create pipeline
    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(myMaterial.getVertexShader(), myMaterial.getFragmentShader())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormat(ptrASwapchain->swapchainFormat)
        .setDepthAttachmentFormat(ptrASwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending()
        .buildPipeline(ptrAContext->device, myMaterial.materialPipelineLayout);
}

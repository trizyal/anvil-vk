// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "DirectionalLight.h"

#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "GPUMesh.h"
#include "ModelLoader.h"
#include "ShaderCompiler.h"
#include "TextureLoader.h"
#include "UIRenderer.h"

void DirectionalLight::initializeProject(VulkanContext& inAnvilContext, VulkanSwapchain& inAnvilSwapchain)
{
    ptrAContext = &inAnvilContext;
    ptrASwapchain = &inAnvilSwapchain;

    const char* modelPath = PROJECT_DIR "/BoxTextured/glTF/BoxTextured.gltf";
    const CPUMesh cubeMesh = ModelLoader::LoadGLTF(modelPath);
    meshBuffer.createAnvilMeshBuffer(*ptrAContext, cubeMesh);

    if (!cubeMesh.texturePath.empty())
    {
        std::cout << "Loading texture: " << cubeMesh.texturePath << std::endl;

        myTexture = TextureLoader::LoadTexture(
            cubeMesh.texturePath,
            *ptrAContext
        );
    }

    // 1. Setup initial light values
    sceneLighting.lightDirection = glm::vec4(-1.0f, -1.0f, -0.5f, 0.0f);     // Sunlight pointing down-left
    sceneLighting.lightColor = glm::vec4(1.0f, 0.95f, 0.8f, 2.0f);   // Warm sunlight, intensity = 2.0
    sceneLighting.ambientColor = glm::vec4(0.08f, 0.1f, 0.15f, 1.0f); // Cool blue sky ambient

    // 2. Create the UBO and upload the initial lighting data using your existing AnvilBuffer!
    sceneUBO.createBuffer(
        ptrAContext->anvilAllocator,
        ptrAContext->anvilDevice,
        &sceneLighting,
        sizeof(SceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );

    // Initialize shader compiler
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);
    loadPipeline();
}

void DirectionalLight::cleanupProject()
{
    if (ptrAContext)
    {
        myTexture.destroyAnvilTexture(ptrAContext);
        sceneUBO.destroyBuffer();
        myMaterial.destroyMaterial();
        meshBuffer.destroyAnvilMeshBuffer();
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
    }
}

void DirectionalLight::loadPipeline()
{
    shaderCompiler.resetSession();

    std::cout << "Creating DirectionalLight pipeline." << std::endl;
    // NO wait idle here. Anvil handled it.
    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
        myMaterial.destroyMaterial();
    }

    // Create shader compilation request
    AnvilShaders::ShaderCompileRequest vReq{"DirectionalLight", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"DirectionalLight", "fragmentMain", AnvilShaders::ST_Fragment};

    // One call for material: Compile, Reflect, Shader Modules, and Build Layouts
    myMaterial.buildMaterial(*ptrAContext, shaderCompiler, vReq, fReq);

    myMaterial.bindUniformBuffer("sceneBuffer", sceneUBO);

    // Bind by name
    if (myTexture.imageView != VK_NULL_HANDLE)
    {
        myMaterial.bindTexture("texture", myTexture);
    }
    myMaterial.updateDescriptorSets();

    auto attributesArray = GPUMesh::getAttributeDescriptions();

    // Vertex Descriptions
    std::vector<VkVertexInputBindingDescription> bindings = {GPUMesh::getBindingDescription()};
    std::vector<VkVertexInputAttributeDescription> attributes =
    {attributesArray[0], attributesArray[1], attributesArray[2]};

    // Create pipeline
    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(myMaterial.vertexShader.get(), myMaterial.fragmentShader.get())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormat(ptrASwapchain->swapchainFormat)
        .setDepthAttachmentFormat(ptrASwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending()
        .buildPipeline(ptrAContext->anvilDevice, myMaterial.materialPipelineLayout, "DirectionalLightPipeline");
}

void DirectionalLight::recordCommands(VkCommandBuffer inCmd, VulkanSwapchain& inAnvilSwapchain)
{
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


    // Simple rotation animation over time
    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime; // Update last frame timestamp

    // Calculate matrices
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(5.0f)); // Make it 5x larger to test

    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    camera.updateCamera(deltaTime);

    const float aspect = static_cast<float>(inAnvilSwapchain.swapchainExtent.width) / static_cast<float>(inAnvilSwapchain.swapchainExtent.height);

    const glm::mat4 projection = camera.getProjectionMatrix(aspect);
    const glm::mat4 view = camera.getViewMatrix();

    UIRenderer::DrawDebugAxis(view);

    PushConstants constants{};
    constants.renderMatrix = projection * view * model;
    constants.modelMatrix = model;
    constants.camera = camera.position;
    vkCmdPushConstants(inCmd, myMaterial.materialPipelineLayout, myMaterial.pushConstantStages, 0, sizeof(PushConstants), &constants);

    vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, myMaterial.materialPipelineLayout, 0, 1, &myMaterial.materialDescriptorSet, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(inCmd, 0, 1, &meshBuffer.vertexBuffer.buffer, &offset);

    // This was VK_INDEX_TYPE_UINT16, but everything else uses 32
    // Caused a bug where half the triangles were not being rendered.
    vkCmdBindIndexBuffer(inCmd, meshBuffer.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw
    vkCmdDrawIndexed(inCmd, meshBuffer.indexCount, 1, 0, 0, 0);
}

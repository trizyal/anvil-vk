// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "TruckModel.h"

#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "GPUMesh.h"
#include "ModelLoader.h"
#include "ShaderCompiler.h"
#include "TextureLoader.h"
#include "UIRenderer.h"

void TruckModel::initializeProject(VulkanContext& inAnvilContext, VulkanSwapchain& inAnvilSwapchain)
{
    pContext = &inAnvilContext;
    pSwapchain = &inAnvilSwapchain;

    const char* modelPath = PROJECT_DIR "/CesiumMilkTruck/glTF/CesiumMilkTruck.gltf";
    models = ModelLoader::LoadGLTF(modelPath);

    for (const auto& cpu_mesh : models.meshes)
    {
        for (const auto& primitive : cpu_mesh.primitives)
        {
            GPUMesh gpu_mesh;
            gpu_mesh.createGPUMesh(*pContext, primitive);

            meshBuffers.push_back(std::move(gpu_mesh));
        }
    }

    for (const auto& texture : models.textures)
    {
        textures.push_back(TextureLoader::LoadTexture(texture.imagePath, inAnvilContext));
        std::cout << "Loading texture: " << texture.imagePath << std::endl;
    }

    // 1. Setup initial light values
    GPUSceneData sceneLighting{};
    sceneLighting.lightDirection = glm::vec4(-1.0f, -1.0f, -0.5f, 0.0f);     // Sunlight pointing down-left
    sceneLighting.lightColor = glm::vec4(1.0f, 0.95f, 0.8f, 2.0f);   // Warm sunlight, intensity = 2.0
    sceneLighting.ambientColor = glm::vec4(0.08f, 0.1f, 0.15f, 1.0f); // Cool blue sky ambient

    myScene.createScene(*pContext);
    myScene.setGPUSceneData(sceneLighting);
    myScene.updateGPUBuffer();

    // Initialize shader compiler
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);
    loadPipeline();
}

void TruckModel::cleanupProject()
{
    if (pContext)
    {
        // myScene.destroyScene();
        for (const auto& texture : textures)
        {
            texture.destroyAnvilTexture(pContext);
        }
        myMaterial.destroyMaterial();
        for (auto & meshBuffer : meshBuffers)
        {
            meshBuffer.destroyGPUMesh();
        }
        vkDestroyPipeline(pContext->anvilDevice, pipeline.pipeline, nullptr);
    }
}

void TruckModel::loadPipeline()
{
    shaderCompiler.resetSession();

    std::cout << "Creating TruckModel pipeline." << std::endl;
    // NO wait idle here. Anvil handled it.
    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(pContext->anvilDevice, pipeline.pipeline, nullptr);
        myMaterial.destroyMaterial();
    }

    // Create shader compilation request
    AnvilShaders::ShaderCompileRequest vReq{"TruckModel", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"TruckModel", "fragmentMain", AnvilShaders::ST_Fragment};

    // One call for material: Compile, Reflect, Shader Modules, and Build Layouts
    myMaterial.buildMaterial(*pContext, shaderCompiler, vReq, fReq);

    myMaterial.bindUniformBuffer("sceneBuffer", myScene.sceneUBO);

    // Bind by name
    if (textures[0].imageView != VK_NULL_HANDLE)
    {
        myMaterial.bindTexture("texture", textures[0]);
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
        .setColorAttachmentFormat(pSwapchain->swapchainFormat)
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending()
        .buildPipeline(pContext->anvilDevice, myMaterial.materialPipelineLayout, "TruckModelPipeline");
}

void TruckModel::recordCommands(VkCommandBuffer inCmd, VulkanSwapchain& inAnvilSwapchain)
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

    // Delta time and Accumulated Total Time calculations
    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    // Track total running time across frames
    static float totalTime = 0.0f;
    totalTime += deltaTime;

    // FIX: Rotate Cube Continuously using accumulated totalTime
    // float rotationSpeed = glm::radians(30.0f); // 30 degrees per second
    // glm::mat4 model = glm::rotate(glm::mat4(1.0f), totalTime * rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = glm::mat4(1.0f);

vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    camera.updateCamera(deltaTime);

    const float aspect = static_cast<float>(inAnvilSwapchain.swapchainExtent.width) / static_cast<float>(inAnvilSwapchain.swapchainExtent.height);
    const glm::mat4 projection = camera.getProjectionMatrix(aspect);
    const glm::mat4 view = camera.getViewMatrix();

    UIRenderer::DrawDebugAxis(view);

    vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, myMaterial.materialPipelineLayout, 0, 1, &myMaterial.materialDescriptorSet, 0, nullptr);

    // Global model transform (e.g., to rotate or move the ENTIRE truck in your game world)
    glm::mat4 globalModelMatrix = glm::mat4(1.0f);
    VkDeviceSize offset = 0;

    // --- NEW: RECURSIVE SCENE GRAPH TRAVERSAL ---

    // Define a recursive lambda function
    auto drawNode = [&](int nodeIndex, glm::mat4 parentMatrix, auto& drawNodeRef) -> void
    {
        // 1. Get the current node (Assuming TruckModel holds onto cpuModel.nodes!)
        const CPUNode& node = this->models.nodes[nodeIndex];

        // 2. Compute this node's absolute transform in the world
        glm::mat4 nodeWorldMatrix = parentMatrix * node.localMatrix;

        // 3. If this node has a mesh attached, draw it!
        if (node.meshIndex >= 0)
        {
            // Update push constants with THIS node's position/rotation
            PushConstants constants{};
            constants.renderMatrix = projection * view * nodeWorldMatrix;
            constants.modelMatrix = nodeWorldMatrix;
            constants.camera = camera.position;

            vkCmdPushConstants(inCmd, myMaterial.materialPipelineLayout, myMaterial.pushConstantStages, 0, sizeof(PushConstants), &constants);

            // Fetch the GPU primitives for this mesh.
            // *NOTE: You need to map node.meshIndex to your meshBuffers here.*
            // Assuming gpuMeshes is a std::vector<std::vector<GPUMesh>> matching cpuModel.meshes
            const auto& primitives = this->meshBuffers[node.meshIndex];

            // In future this should be a list of lists
            // for (const GPUMesh& meshBuffer : primitives)
            {
                auto& meshBuffer = primitives;
                vkCmdBindVertexBuffers(inCmd, 0, 1, &meshBuffer.vertexBuffer.buffer, &offset);
                vkCmdBindIndexBuffer(inCmd, meshBuffer.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(inCmd, meshBuffer.indexCount, 1, 0, 0, 0);
            }
        }

        // 4. Recursively draw all children (like the wheels attached to the axles)
        for (int childIndex : node.children)
        {
            drawNodeRef(childIndex, nodeWorldMatrix, drawNodeRef);
        }
    };

    // --- START THE TRAVERSAL ---

    // Start drawing from the root nodes
    for (int rootNodeIndex : this->models.sceneRootNodes)
    {
        drawNode(rootNodeIndex, globalModelMatrix, drawNode);
    }
}

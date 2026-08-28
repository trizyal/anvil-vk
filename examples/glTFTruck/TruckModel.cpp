// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "TruckModel.h"

#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "GPUMesh.h"
#include "CPUModel.h"
#include "ShaderCompiler.h"
#include "TextureLoader.h"
#include "UIRenderer.h"
#include "UIElements.h"

void TruckModel::initializeProject(VulkanContext& inAnvilContext, Swapchain& inAnvilSwapchain)
{
    pContext = &inAnvilContext;
    pSwapchain = &inAnvilSwapchain;

    const char* modelPath = PROJECT_DIR "/CesiumMilkTruck/glTF/CesiumMilkTruck.gltf";
    cpuModel = ModelLoader::LoadGLTF(modelPath);

    // Setup initial light values
    GlobalSceneData sceneLighting{};
    sceneLighting.lightDirection = glm::vec4(-1.0f, -1.0f, -0.5f, 0.0f); // Sunlight pointing down-left
    sceneLighting.lightColor = glm::vec4(1.0f, 0.95f, 0.8f, 2.0f); // Warm sunlight, intensity = 2.0
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
        vkDeviceWaitIdle(pContext->device);

        gpuModel.destroyGPUModel();
        myMaterial.destroyMaterial();

        if (pipeline.pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
            pipeline.pipeline = VK_NULL_HANDLE;
        }

        shaderCompiler.shutdownShaderCompiler();
    }
}

void TruckModel::loadPipeline()
{
    std::cout << "Creating TruckModel pipeline." << std::endl;

    shaderCompiler.resetSession();

    // NO wait idle here. Anvil handled it.
    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
        pipeline.pipeline = VK_NULL_HANDLE;
        myMaterial.destroyMaterial();
    }

    // Create shader compilation request
    AnvilShaders::ShaderCompileRequest vReq{"TruckModel", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"TruckModel", "fragmentMain", AnvilShaders::ST_Fragment};

    // One call for material: Compile, Reflect, Shader Modules, and Build Layouts
    myMaterial.buildMaterial(*pContext, shaderCompiler, vReq, fReq);

    auto attributesArray = GPUMesh::GetAttributeDescriptionsArray3();

    // Vertex Descriptions
    std::vector<VkVertexInputBindingDescription> bindings = {GPUMesh::GetBindingDescription()};
    std::vector<VkVertexInputAttributeDescription> attributes =
    {attributesArray[0], attributesArray[1], attributesArray[2]};

    // Create pipeline
    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(myMaterial.getVertexShader(), myMaterial.getFragmentShader())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormat(pSwapchain->swapchainFormat)
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending()
        .buildPipeline(pContext->device, myMaterial.materialPipelineLayout DNAME("TruckModelPipeline"));

    gpuModel.createGPUModel( *pContext, cpuModel, myMaterial, "sceneBuffer", myScene.sceneUBO, "texture");
}

void TruckModel::recordCommands(VkCommandBuffer inCmd, Swapchain& inAnvilSwapchain)
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

    camera.updateCamera(deltaTime);

    // ----------------------------------------
    // ANIMATION PROCESSING

    if (!cpuModel.animations.empty() && cpuModel.animations[0].duration > 0.0f)
    {
        animationTime += deltaTime;
        animationTime = fmod(animationTime, cpuModel.animations[0].duration);

        // Update matrices in the CPU
        cpuModel.applyAnimation(0, animationTime);

        // Update the matrices in the GPU buffers.
        gpuModel.updateTransforms(cpuModel);
    }

    // ----------------------------------------

    const float aspect =
        static_cast<float>(inAnvilSwapchain.swapchainExtent.width) /
        static_cast<float>(inAnvilSwapchain.swapchainExtent.height);

    const glm::mat4 projection = camera.getProjectionMatrix(aspect);
    const glm::mat4 view = camera.getViewMatrix();

    UI::RenderWorldAxes(view);

    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    VkDeviceSize offset = 0;

    for (const GPUModelDrawItem& draw_item : gpuModel.drawItems)
    {
        if (draw_item.gpuMeshIndex >= gpuModel.gpuMeshes.size())
        {
            continue;
        }

        const GPUMesh& gpu_mesh = gpuModel.gpuMeshes[draw_item.gpuMeshIndex];

        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
        glm::vec4 base_color_factor = glm::vec4(1.0f);

        if (draw_item.gpuMaterialIndex >= 0 &&
            draw_item.gpuMaterialIndex < static_cast<int>(gpuModel.gpuMaterials.size()))
        {
            const GPUModelMaterial& material = gpuModel.gpuMaterials[draw_item.gpuMaterialIndex];
            descriptor_set = material.instance.descriptorSet; // Get the instance's unique set
            base_color_factor = material.baseColorFactor;
        }

        // Bind the specific descriptor set for this material (textures + scene UBO)
        if (descriptor_set != VK_NULL_HANDLE)
        {
            vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, myMaterial.materialPipelineLayout,
                0, 1, &descriptor_set, 0, nullptr);
        }

        // Update push constants (Transform matrices + base color)
        PushConstants constants{};
        constants.renderMatrix = projection * view * draw_item.worldMatrix;
        constants.modelMatrix = draw_item.worldMatrix;
        constants.camera = glm::vec4(camera.position, 1.0f);
        constants.baseColorFactor = base_color_factor;

        vkCmdPushConstants(inCmd, myMaterial.materialPipelineLayout, myMaterial.pushConstantStages, 0,
            sizeof(PushConstants), &constants);

        // Bind buffers and draw
        vkCmdBindVertexBuffers(inCmd, 0, 1, &gpu_mesh.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(inCmd, gpu_mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(inCmd, gpu_mesh.indexCount, 1, 0, 0, 0);
    }
}

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "BoxAnimated.h"

#include <iostream>
#include <stdexcept>

#include <glm/glm.hpp>

#include "AnvilRenderer.h"
#include "GPUMesh.h"
#include "CPUModel.h"
#include "UIRenderer.h"
#include "ShaderCompiler.h"
#include "UIElements.h"

void BoxAnimated::initializeProject(VulkanContext& inContext, Swapchain& inSwapchain)
{
    pContext = &inContext;
    pSwapchain = &inSwapchain;

    const char* modelPath = PROJECT_DIR "/BoxAnimated/glTF/BoxAnimated.gltf";
    cpuModel.loadGLTF(modelPath);

    // Setup initial light values
    DirectionalLighting sceneLighting{};
    sceneLighting.lightDirection = glm::vec4(-1.0f, -1.0f, -0.5f, 0.0f); // Sunlight pointing down-left
    sceneLighting.lightColor = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f); // Warm sunlight, intensity = 2.0
    sceneLighting.ambientColor = glm::vec4(0.08f, 0.1f, 0.15f, 1.0f); // Cool blue sky ambient

    boxScene.createScene(*pContext);
    boxScene.setGPUSceneData(sceneLighting);
    boxScene.updateGPUBuffer();

    // Initialize shader compiler
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);
    loadPipeline();
}

void BoxAnimated::cleanupProject()
{
    if (pContext)
    {
        vkDeviceWaitIdle(pContext->device);

        gpuModel.destroyGPUModel();
        boxMaterial.destroyMaterial();

        if (pipeline.pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
            pipeline.pipeline = VK_NULL_HANDLE;
        }

        shaderCompiler.shutdownShaderCompiler();
    }
}

void BoxAnimated::loadPipeline()
{
    std::cout << "Creating BoxAnimated pipeline." << std::endl;

    shaderCompiler.resetSession();
    if (pipeline.pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
        pipeline.pipeline = VK_NULL_HANDLE;
        boxMaterial.destroyMaterial();
    }

    // Create shader compilation request
    AnvilShaders::ShaderCompileRequest vReq{"BoxAnimated", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"BoxAnimated", "fragmentMain", AnvilShaders::ST_Fragment};

    // One call for material: Compile, Reflect, Shader Modules, and Build Layouts
    boxMaterial.buildMaterial(*pContext, shaderCompiler, vReq, fReq);

    const auto attributesArray = GPUMesh::get3AttributeDescriptions();

    // Vertex Descriptions
    std::vector<VkVertexInputBindingDescription> bindings = {GPUMesh::getBindingDescription()};
    std::vector<VkVertexInputAttributeDescription> attributes =
    {attributesArray[0], attributesArray[1], attributesArray[2]};

    // Create pipeline
    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(boxMaterial.vertexShader.get(), boxMaterial.fragmentShader.get())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormat(pSwapchain->swapchainFormat)
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending()
        .buildPipeline(pContext->device, boxMaterial.materialPipelineLayout, "BoxAnimatedPipeline");

    gpuModel.createGPUModel( *pContext, cpuModel, boxMaterial, "sceneBuffer", boxScene.sceneUBO, "texture");
}

void BoxAnimated::recordCommands(VkCommandBuffer inCmd, Swapchain& inSwapchain)
{
        // Set Dynamic States required by your AnvilPipelineBuilder
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(inSwapchain.swapchainExtent.width);
    viewport.height = static_cast<float>(inSwapchain.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(inCmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = inSwapchain.swapchainExtent;
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
        static_cast<float>(inSwapchain.swapchainExtent.width) /
        static_cast<float>(inSwapchain.swapchainExtent.height);

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
            vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, boxMaterial.materialPipelineLayout,
                0, 1, &descriptor_set, 0, nullptr);
        }

        // Update push constants (Transform matrices + base color)
        PushConstants constants{};
        constants.renderMatrix = projection * view * draw_item.worldMatrix;
        constants.modelMatrix = draw_item.worldMatrix;
        constants.camera = glm::vec4(camera.position, 1.0f);
        constants.baseColorFactor = base_color_factor;

        vkCmdPushConstants(inCmd, boxMaterial.materialPipelineLayout, boxMaterial.pushConstantStages, 0,
            sizeof(PushConstants), &constants);

        // Bind buffers and draw
        vkCmdBindVertexBuffers(inCmd, 0, 1, &gpu_mesh.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(inCmd, gpu_mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(inCmd, gpu_mesh.indexCount, 1, 0, 0, 0);
        AnvilRenderer::engineStats.drawCalls++;
        AnvilRenderer::engineStats.primitiveCount += (gpu_mesh.indexCount/3);
    }
}

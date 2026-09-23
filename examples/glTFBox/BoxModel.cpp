// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "BoxModel.h"

#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "AnvilRenderer.h"
#include "GPUMesh.h"
#include "CPUModel.h"
#include "ShaderCompiler.h"
#include "UIElements.h"

void BoxModel::initializeProject(VulkanContext& inAnvilContext, Swapchain& inAnvilSwapchain)
{
    pContext = &inAnvilContext;
    pSwapchain = &inAnvilSwapchain;

    // Use CPUModel to parse the file instead of the deprecated LoadSingleMeshGLTF Loader
    const char* modelPath = PROJECT_DIR "/Box/glTF/Box.gltf";
    cpuModel.loadGLTF(modelPath);

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
    if (pContext)
    {
        vkDeviceWaitIdle(pContext->device);

        gpuModel.destroyGPUModel();
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

void BoxModel::loadPipeline()
{
    std::cout << "Creating BoxModel pipeline." << std::endl;

    shaderCompiler.resetSession();

    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
        pipeline.pipeline = VK_NULL_HANDLE;
        myMaterial.destroyMaterial();
        myProgram.destroyProgram();
    }

    // Create shader compilation request
    AnvilShaders::ShaderCompileRequest vReq{"BoxModel", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"BoxModel", "fragmentMain", AnvilShaders::ST_Fragment};

    // Split build process to build Program then Material
    myProgram.buildProgram(*pContext, shaderCompiler, vReq, fReq);
    myMaterial.buildMaterialFromProgram(*pContext, myProgram);

    // Use initializer list for non-deprecated GetAttributeDescriptions
    const auto attributes = GPUMesh::GetAttributeDescriptions({POSITION, NORMAL});
    std::vector<VkVertexInputBindingDescription> bindings = {GPUMesh::GetBindingDescription()};

    // Create pipeline
    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(myMaterial.getVertexShader(), myMaterial.getFragmentShader())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormats({pSwapchain->swapchainFormat}) // Wrapped format in {}
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending()
        .buildPipeline(pContext->device, myMaterial.materialPipelineLayout DNAME("BoxModelPipeline"));

    // Upload to GPU
    gpuModel.createGPUModel(*pContext, cpuModel, myMaterial);
}

void BoxModel::recordCommands(VkCommandBuffer inCmd, Swapchain &inAnvilSwapchain)
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

    // Calculate C++ Transforms
    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    static float totalTime = 0.0f;
    totalTime += deltaTime;

    camera.updateCamera(deltaTime);

    // Update node transforms smoothly over time
    if (!cpuModel.nodes.empty())
    {
        float rotationSpeed = glm::radians(45.0f);
        cpuModel.nodes[0].rotation = glm::angleAxis(totalTime * rotationSpeed, glm::vec3(0.5f, 1.0f, 0.0f));
        cpuModel.nodes[0].localMatrix = glm::translate(glm::mat4(1.0f), cpuModel.nodes[0].translation) *
                                        glm::mat4_cast(cpuModel.nodes[0].rotation) *
                                        glm::scale(glm::mat4(1.0f), cpuModel.nodes[0].scale);

        cpuModel.updateAllMatrices();
        gpuModel.updateTransforms(cpuModel);
    }

    const float aspect = static_cast<float>(inAnvilSwapchain.swapchainExtent.width) / static_cast<float>(inAnvilSwapchain.swapchainExtent.height);

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

        if (draw_item.gpuMaterialIndex >= 0 && draw_item.gpuMaterialIndex < static_cast<int>(gpuModel.gpuMaterials.size()))
        {
            const GPUModelMaterial& material = gpuModel.gpuMaterials[draw_item.gpuMaterialIndex];
            descriptor_set = material.instance.descriptorSet;
            base_color_factor = material.baseColorFactor;
        }

        std::vector<VkDescriptorSet> sets_to_bind;
        if (gpuModel.modelSet.descriptorSet != VK_NULL_HANDLE)
        {
            sets_to_bind.push_back(gpuModel.modelSet.descriptorSet);
        }
        if (descriptor_set != VK_NULL_HANDLE)
        {
            sets_to_bind.push_back(descriptor_set);
        }

        if (!sets_to_bind.empty())
        {
            // Note: Since this shader has no textures or UBOs, this block likely won't even execute!
            vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, myMaterial.materialPipelineLayout,
                1, static_cast<uint32_t>(sets_to_bind.size()), sets_to_bind.data(), 0, nullptr);
        }

        ProjectPushConstants constants{};
        constants.renderMatrix = projection * view * draw_item.worldMatrix;
        constants.modelMatrix = draw_item.worldMatrix;
        constants.baseColorFactor = base_color_factor;

        vkCmdPushConstants(inCmd, myMaterial.materialPipelineLayout, myMaterial.pushConstantStages, 0,
            sizeof(ProjectPushConstants), &constants);

        // Bind buffers and draw
        vkCmdBindVertexBuffers(inCmd, 0, 1, &gpu_mesh.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(inCmd, gpu_mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(inCmd, gpu_mesh.indexCount, 1, 0, 0, 0);

        AnvilRenderer::engineStats.drawCalls++;
        AnvilRenderer::engineStats.primitiveCount += (gpu_mesh.indexCount/3);
    }
}
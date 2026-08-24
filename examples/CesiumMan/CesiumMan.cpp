// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "CesiumMan.h"

#include "AnvilRenderer.h"
#include "UIElements.h"

void CesiumMan::initializeProject(VulkanContext& inContext, Swapchain& inSwapchain)
{
    pContext = &inContext;
    pSwapchain = &inSwapchain;

    const char* modelPath = PROJECT_DIR "/CesiumMan/glTF/CesiumMan.gltf";
    cpuModel.loadGLTF(modelPath);

    // Setup directional light
    DirectionalLighting sceneLighting{};
    sceneLighting.lightDirection = glm::vec4(-0.5f, -1.0f, -0.3f, 0.0f);
    sceneLighting.lightColor = glm::vec4(1.2f, 1.2f, 1.2f, 1.0f);
    sceneLighting.ambientColor = glm::vec4(0.15f, 0.15f, 0.2f, 1.0f);

    cesiumScene.createScene(*pContext);
    cesiumScene.setGPUSceneData(sceneLighting);
    cesiumScene.updateGPUBuffer();

    // Initialize shader compiler
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);
    loadPipeline();
}

void CesiumMan::cleanupProject()
{
    if (pContext)
    {
        vkDeviceWaitIdle(pContext->device);

        gpuModel.destroyGPUModel();
        cesiumMaterial.destroyMaterial();

        if (pipeline.pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
            pipeline.pipeline = VK_NULL_HANDLE;
        }

        shaderCompiler.shutdownShaderCompiler();
    }
}

void CesiumMan::loadPipeline()
{
    shaderCompiler.resetSession();
    if (pipeline.pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
        pipeline.pipeline = VK_NULL_HANDLE;
        cesiumMaterial.destroyMaterial();
    }

    AnvilShaders::ShaderCompileRequest vReq{"CesiumMan", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"CesiumMan", "fragmentMain", AnvilShaders::ST_Fragment};

    // Compile, reflect, and build bindings (handles textures, UBOs, and SSBOs automatically)
    cesiumMaterial.buildMaterial(*pContext, shaderCompiler, vReq, fReq);

    globalSet = cesiumMaterial.allocateSet(0);
    globalSet.bindUniformBuffer("sceneBuffer", cesiumScene.sceneUBO);
    globalSet.updateDescriptorSets();

    const auto attributes = GPUMesh::getAttributeDescriptions();
    std::vector<VkVertexInputBindingDescription> bindings = {GPUMesh::getBindingDescription()};

    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(cesiumMaterial.vertexShader.get(), cesiumMaterial.fragmentShader.get())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormat(pSwapchain->swapchainFormat)
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending()
        .buildPipeline(pContext->device, cesiumMaterial.materialPipelineLayout, "CesiumManPipeline");

    gpuModel.createGPUModel(*pContext, cpuModel, cesiumMaterial);
}

void CesiumMan::recordCommands(VkCommandBuffer inCmd, Swapchain& inSwapchain)
{
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

    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    camera.updateCamera(deltaTime);

    // Update Animation
    if (!cpuModel.animations.empty() && cpuModel.animations[0].duration > 0.0f)
    {
        animationTime += deltaTime;
        animationTime = fmod(animationTime, cpuModel.animations[0].duration);

        cpuModel.applyAnimation(0, animationTime);
        gpuModel.updateTransforms(cpuModel);
    }

    if (!cpuModel.skins.empty())
    {
        gpuModel.updateJoints(cpuModel);
    }

    const float aspect = static_cast<float>(inSwapchain.swapchainExtent.width) /
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
            descriptor_set = gpuModel.gpuMaterials[draw_item.gpuMaterialIndex].instance.descriptorSet;
            base_color_factor = gpuModel.gpuMaterials[draw_item.gpuMaterialIndex].baseColorFactor;
        }

        // Bind all active sets
        std::vector<VkDescriptorSet> sets_to_bind;
        if (globalSet.descriptorSet != VK_NULL_HANDLE)
        {
            sets_to_bind.push_back(globalSet.descriptorSet);
        }
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
            vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cesiumMaterial.materialPipelineLayout,
                0, static_cast<uint32_t>(sets_to_bind.size()), sets_to_bind.data(), 0, nullptr);
        }

        PushConstants constants{};
        constants.renderMatrix = projection * view * draw_item.worldMatrix;
        constants.modelMatrix = draw_item.worldMatrix;
        constants.camera = glm::vec4(camera.position, 1.0f);
        constants.baseColorFactor = base_color_factor;

        vkCmdPushConstants(inCmd, cesiumMaterial.materialPipelineLayout, cesiumMaterial.pushConstantStages, 0,
            sizeof(PushConstants), &constants);

        vkCmdBindVertexBuffers(inCmd, 0, 1, &gpu_mesh.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(inCmd, gpu_mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(inCmd, gpu_mesh.indexCount, 1, 0, 0, 0);

        AnvilRenderer::engineStats.drawCalls++;
        AnvilRenderer::engineStats.primitiveCount += (gpu_mesh.indexCount / 3);
    }
}


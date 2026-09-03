// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Sponza.h"

#include <iostream>

#include "AnvilRenderer.h"
#include "UIElements.h"

void Sponza::initializeProject(VulkanContext& inContext, Swapchain& inSwapchain)
{
    std::cout << "Initialize project" << std::endl;
    pContext = &inContext;
    pSwapchain = &inSwapchain;

    // Adjust camera to look down the main hall of Sponza
    camera.position = glm::vec3(0.0f, 2.0f, 0.0f);
    camera.cameraSpeed = 15.0f;

    const char* modelPath = ASSETS_DIR "/models/Sponza/glTF/Sponza.gltf";
    cpuModel.loadGLTF(modelPath);

    GlobalSceneData scene_data{};
    scene_data.lightDirection = glm::vec4(-0.2f, -1.0f, -0.2f, 0.0f);
    scene_data.lightColor = glm::vec4(1.5f, 1.4f, 1.2f, 1.0f);
    scene_data.ambientColor = glm::vec4(0.2f, 0.25f, 0.3f, 1.0f);
    scene_data.debugViewMode = 0;

    sponzaScene.createScene(*pContext);
    sponzaScene.setGPUSceneData(scene_data);
    sponzaScene.updateGPUBuffer();

    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);
    loadPipeline();

    std::cout << "Project initialization completed." << std::endl;
}

void Sponza::cleanupProject()
{
    if (pContext)
    {
        vkDeviceWaitIdle(pContext->device);

        gpuModel.destroyGPUModel();
        sponzaMaterial.destroyMaterial();
        sponzaProgram.destroyProgram();

        if (pipeline.pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
            pipeline.pipeline = VK_NULL_HANDLE;
        }

        shaderCompiler.shutdownShaderCompiler();
    }
}

bool Sponza::loadPipeline(std::string* outErrorMessage)
{
    std::cout << "Loading Pipeline." << std::endl;
    shaderCompiler.resetSession();

    AnvilShaders::ShaderCompileRequest v_req{"Sponza", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest f_req{"Sponza", "fragmentMain", AnvilShaders::ST_Fragment};

    // Try building new program into a temporary instance
    ShaderProgram new_program;
    if (!new_program.buildProgram(*pContext, shaderCompiler, v_req, f_req, outErrorMessage))
    {
        std::cerr << "[Sponza] Shader reload failed. Retaining old pipeline." << std::endl;
        return false;
    }

    // Compilation Succeeded! Destroy old resources safely
    if (pipeline.pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
        pipeline.pipeline = VK_NULL_HANDLE;
        sponzaMaterial.destroyMaterial();
        sponzaProgram.destroyProgram();
    }


    sponzaProgram = std::move(new_program);
    sponzaMaterial.buildMaterialFromProgram(*pContext, sponzaProgram);

    // Setup Set 0
    globalSet = sponzaMaterial.allocateSet(0);
    globalSet.bindUniformBuffer("sceneBuffer", sponzaScene.sceneUBO);
    globalSet.updateDescriptorSets();

    auto attributes = GPUMesh::GetAttributeDescriptions(
        {POSITION, NORMAL, TANGENT, UV }
    );

    std::vector<VkVertexInputBindingDescription> bindings = {GPUMesh::GetBindingDescription()};

    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(sponzaMaterial.pActiveProgram->vertexShader.get(), sponzaMaterial.pActiveProgram->fragmentShader.get())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormat(pSwapchain->swapchainFormat)
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE) // NO CULLING FOR CURTAINS
        .disableBlending()
        .buildPipeline(pContext->device, sponzaMaterial.materialPipelineLayout DNAME("SponzaPipeline"));

    gpuModel.createGPUModel(*pContext, cpuModel, sponzaMaterial);

    std::cout << "Pipeline loading completed." << std::endl;
    return true;
}

void Sponza::recordCommands(VkCommandBuffer inCmd, Swapchain& inSwapchain)
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

    const float aspect = static_cast<float>(inSwapchain.swapchainExtent.width) /
                         static_cast<float>(inSwapchain.swapchainExtent.height);

    const glm::mat4 projection = camera.getProjectionMatrix(aspect);
    const glm::mat4 view = camera.getViewMatrix();

    UI::RenderWorldAxes(view);

    // Render the Debug Menu and update the GPU immediately if the user clicks a new mode
    if (UI::RenderDebugMenu(sponzaScene.data.debugViewMode))
    {
        sponzaScene.setGPUSceneData(sponzaScene.data);
        sponzaScene.updateGPUBuffer();
    }

    gpuModel.updateTransforms(cpuModel);
    const glm::mat4 view_projection = projection * view; // Calculate once!

    Frustum cameraFrustum{};
    cameraFrustum.extractPlanes(view_projection);

    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    VkDeviceSize offset = 0;
    for (size_t i = 0; i < gpuModel.drawItems.size(); ++i)
    {
        const GPUModelDrawItem& draw_item = gpuModel.drawItems[i];
        if (draw_item.gpuMeshIndex >= gpuModel.gpuMeshes.size())
        {
            continue;
        }

        // Fast AABB World Transform & Frustum Check
        glm::vec3 center = draw_item.localBounds.getCenter();
        glm::vec3 extents = draw_item.localBounds.getExtents();
        glm::vec3 worldCenter = glm::vec3(draw_item.worldMatrix * glm::vec4(center, 1.0f));
        glm::mat3 absModel = glm::mat3(
            glm::abs(draw_item.worldMatrix[0]),
            glm::abs(draw_item.worldMatrix[1]),
            glm::abs(draw_item.worldMatrix[2])
        );
        glm::vec3 worldExtents = absModel * extents;

        AABB worldAABB{ .min = worldCenter - worldExtents, .max = worldCenter + worldExtents };

        static bool culling = true; // TODO: this culling should be toggleable
        if (!cameraFrustum.contains(worldAABB) && culling)
        {
            continue; // culled
        }

        const GPUMesh& gpu_mesh = gpuModel.gpuMeshes[draw_item.gpuMeshIndex];

        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;

        if (draw_item.gpuMaterialIndex >= 0 &&
            draw_item.gpuMaterialIndex < static_cast<int>(gpuModel.gpuMaterials.size()))
        {
            descriptor_set = gpuModel.gpuMaterials[draw_item.gpuMaterialIndex].instance.descriptorSet;
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
            vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, sponzaMaterial.materialPipelineLayout,
                0, static_cast<uint32_t>(sets_to_bind.size()), sets_to_bind.data(), 0, nullptr);
        }

        PushConstants constants{};
        constants.viewProjection = view_projection;
        constants.camera = glm::vec4(camera.position, 1.0f);
        constants.objectIndex = static_cast<uint32_t>(i); // Map to SSBO index

        vkCmdPushConstants(inCmd, sponzaMaterial.materialPipelineLayout, sponzaMaterial.pushConstantStages, 0,
            sizeof(PushConstants), &constants);

        vkCmdBindVertexBuffers(inCmd, 0, 1, &gpu_mesh.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(inCmd, gpu_mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(inCmd, gpu_mesh.indexCount, 1, 0, 0, 0);

        AnvilRenderer::engineStats.drawCalls++;
        AnvilRenderer::engineStats.primitiveCount += (gpu_mesh.indexCount / 3);
    }
}

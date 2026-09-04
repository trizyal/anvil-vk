// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "SponzaDeferred.h"

#include <iostream>

#include "AnvilRenderer.h"
#include "UIElements.h"

void SponzaDeferred::initializeProject(VulkanContext& inContext, Swapchain& inSwapchain)
{
    std::cout << "Initialize project" << std::endl;
    pContext = &inContext;
    pSwapchain = &inSwapchain;

    // Adjust camera to look down the main hall of Sponza
    camera.position = glm::vec3(0.0f, 2.0f, 0.0f);
    camera.cameraSpeed = 15.0f;

    const char* modelPath = ASSETS_DIR "/models/Sponza/glTF/Sponza.gltf";
    cpuModel.loadGLTF(modelPath);

    gBuffer.create(*pContext, inSwapchain.swapchainExtent);

    GlobalSceneData scene_data{};
    scene_data.lightDirection = glm::vec4(-0.2f, -1.0f, -0.2f, 0.0f);
    scene_data.lightColor = glm::vec4(1.5f, 1.4f, 1.2f, 1.0f);
    scene_data.ambientColor = glm::vec4(0.2f, 0.25f, 0.3f, 1.0f);
    scene_data.debugViewMode = 0;

    sponzaScene.createScene(*pContext);
    sponzaScene.setGPUSceneData(scene_data);
    sponzaScene.updateGPUBuffer();

    shaderCompiler.initializeShaderCompiler();
    shaderCompiler.addSearchPath(PROJECT_DIR);

    loadPipelines();
}

void SponzaDeferred::cleanupProject()
{
    if (pContext)
    {
        vkDeviceWaitIdle(pContext->device);

        gBuffer.destroy();
        gpuModel.destroyGPUModel();

        material_Geo.destroyMaterial();
        shaderProgram_Geo.destroyProgram();
        pipeline_Geo.destroy(pContext);

        material_Light.destroyMaterial();
        shaderProgram_Light.destroyProgram();
        pipeline_Light.destroy(pContext);

        shaderCompiler.shutdownShaderCompiler();
    }
}

bool SponzaDeferred::loadPipelines(std::string* outErrorMessage)
{
    std::cout << "Loading Pipelines." << std::endl;
    shaderCompiler.resetSession();

    if (!loadGeometryPipeline(outErrorMessage))
    {
        return false;
    }

    gpuModel.createGPUModel(*pContext, cpuModel, material_Geo);

    if (!loadLightingPipeline(outErrorMessage))
    {
        return false;
    }

    // Setup Lighting Descriptor Set
    sceneLightingSet = material_Light.allocateSet(0);
    sceneLightingSet.bindTexture("gAlbedo", gBuffer.albedo);
    sceneLightingSet.bindTexture("gNormal", gBuffer.normal);
    sceneLightingSet.bindTexture("gPBR", gBuffer.pbr);
    sceneLightingSet.bindTexture("gWorldPosition", gBuffer.worldPosition);
    sceneLightingSet.bindUniformBuffer("sceneBuffer", sponzaScene.sceneUBO);
    sceneLightingSet.updateDescriptorSets();

    std::cout << "Finished Loading Pipelines." << std::endl;
    return true;
}

bool SponzaDeferred::loadGeometryPipeline(std::string* outErrorMessage)
{
    std::cout << "Loading Geometry Pipeline." << std::endl;

    AnvilShaders::ShaderCompileRequest v_req{"SponzaGeometry", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest f_req{"SponzaGeometry", "fragmentMain", AnvilShaders::ST_Fragment};

    // Try building new program into a temporary instance
    ShaderProgram new_program;
    if (!new_program.buildProgram(*pContext, shaderCompiler, v_req, f_req, outErrorMessage))
    {
        std::cerr << "[Sponza] Geometry Shader reload failed. Retaining old pipeline." << std::endl;
        return false;
    }

    pipeline_Geo.destroy(pContext);
    material_Geo.destroyMaterial();
    shaderProgram_Geo.destroyProgram();

    shaderProgram_Geo = std::move(new_program);
    material_Geo.buildMaterialFromProgram(*pContext, shaderProgram_Geo);

    // Geometry Pipeline uses 4 Color Attachments (Albedo, Normal, PBR, WorldPosition)
    std::vector<VkFormat> attachments = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R16G16B16A16_SFLOAT
    };

    auto bindings = {GPUMesh::GetBindingDescription()};
    auto attributes = GPUMesh::GetAttributeDescriptions(
        {POSITION, NORMAL, TANGENT, UV }
    );

    PipelineBuilder builder;
    pipeline_Geo = builder.setShaders(material_Geo.getVertexShader(), material_Geo.getFragmentShader())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormats(attachments)
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending()
        .buildPipeline(pContext->device, material_Geo.materialPipelineLayout DNAME("Geometry Pipeline"));

    std::cout << "Finished Loading Geometry Pipeline." << std::endl;
    return true;
}

bool SponzaDeferred::loadLightingPipeline(std::string* outErrorMessage)
{
    std::cout << "Loading Lighting Pipeline." << std::endl;

    AnvilShaders::ShaderCompileRequest v_req{"SponzaLighting", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest f_req{"SponzaLighting", "fragmentMain", AnvilShaders::ST_Fragment};

    // Try building new program into a temporary instance
    ShaderProgram new_program;
    if (!new_program.buildProgram(*pContext, shaderCompiler, v_req, f_req, outErrorMessage))
    {
        std::cerr << "[Sponza] Lighting Shader reload failed. Retaining old pipeline." << std::endl;
        return false;
    }

    pipeline_Light.destroy(pContext);
    material_Light.destroyMaterial();
    shaderProgram_Light.destroyProgram();

    shaderProgram_Light = std::move(new_program);
    material_Light.buildMaterialFromProgram(*pContext, shaderProgram_Light);

    PipelineBuilder builder;
    pipeline_Light = builder.setShaders(material_Light.getVertexShader(), material_Light.getFragmentShader())
        .setVertexInput({}, {}) // Empty vertex inputs
        .setColorAttachmentFormats({pSwapchain->swapchainFormat})
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(false, VK_COMPARE_OP_ALWAYS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending()
        .buildPipeline(pContext->device, material_Light.materialPipelineLayout DNAME("Lighting Pipeline"));

    return true;
}

void SponzaDeferred::recordGeometryPass(VkCommandBuffer inCmd, const Swapchain& inSwapchain)
{
    // Handle Window Resize
    if (gBuffer.currentExtent.width != inSwapchain.swapchainExtent.width || gBuffer.currentExtent.height != inSwapchain.swapchainExtent.height)
    {
        gBuffer.create(*pContext, inSwapchain.swapchainExtent);
        sceneLightingSet.bindTexture("gAlbedo", gBuffer.albedo);
        sceneLightingSet.bindTexture("gNormal", gBuffer.normal);
        sceneLightingSet.bindTexture("gPBR", gBuffer.pbr);
        sceneLightingSet.bindTexture("gWorldPosition", gBuffer.worldPosition);
        sceneLightingSet.updateDescriptorSets();
    }

    // TODO: this delta time implementation needs some work
    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    camera.updateCamera(deltaTime);
    gpuModel.updateTransforms(cpuModel);

    // Transition G-Buffer to Attachment Optimal
    AnvilRenderer::transitionImageLayout(inCmd, gBuffer.albedo.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::transitionImageLayout(inCmd, gBuffer.normal.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::transitionImageLayout(inCmd, gBuffer.pbr.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::transitionImageLayout(inCmd, gBuffer.worldPosition.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::transitionImageLayout(inCmd, gBuffer.depth.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    // Begin Geometry Rendering Pass
    const auto color_attachments = gBuffer.getRenderingAttachments();
    const auto depth_attachment = gBuffer.getDepthAttachmentInfo();
    VkRenderingInfo render_info{};
    render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    render_info.renderArea.offset = {0, 0};
    render_info.renderArea.extent = inSwapchain.swapchainExtent;
    render_info.layerCount = 1;
    render_info.colorAttachmentCount = static_cast<uint32_t>(color_attachments.size());
    render_info.pColorAttachments = color_attachments.data();
    render_info.pDepthAttachment = &depth_attachment;

    vkCmdBeginRendering(inCmd, &render_info);

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

    const float aspect = static_cast<float>(inSwapchain.swapchainExtent.width) /
                         static_cast<float>(inSwapchain.swapchainExtent.height);

    const glm::mat4 projection = camera.getProjectionMatrix(aspect);
    const glm::mat4 view = camera.getViewMatrix();
    const glm::mat4 view_projection = projection * view;

    Frustum cameraFrustum{};
    cameraFrustum.extractPlanes(view_projection);

    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_Geo.pipeline);

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

        std::vector<VkDescriptorSet> sets = {
            gpuModel.modelSet.descriptorSet,
            gpuModel.gpuMaterials[draw_item.gpuMaterialIndex].instance.descriptorSet
        };

        vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material_Geo.materialPipelineLayout, 1, 2, sets.data(), 0, nullptr);

        PushConstants constants{};
        constants.viewProjection = view_projection;
        constants.camera = glm::vec4(camera.position, 1.0f);
        constants.objectIndex = static_cast<uint32_t>(i); // Map to SSBO index
        vkCmdPushConstants(inCmd, material_Geo.materialPipelineLayout, material_Geo.pushConstantStages, 0, sizeof(PushConstants), &constants);

        const GPUMesh& mesh = gpuModel.gpuMeshes[draw_item.gpuMeshIndex];
        vkCmdBindVertexBuffers(inCmd, 0, 1, &mesh.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(inCmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(inCmd, mesh.indexCount, 1, 0, 0, 0);

        AnvilRenderer::engineStats.drawCalls++;
        AnvilRenderer::engineStats.primitiveCount += (mesh.indexCount / 3);
    }

    vkCmdEndRendering(inCmd);

    // Transition G-Buffer to Shader Read
    AnvilRenderer::transitionImageLayout(inCmd, gBuffer.albedo.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    AnvilRenderer::transitionImageLayout(inCmd, gBuffer.normal.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    AnvilRenderer::transitionImageLayout(inCmd, gBuffer.pbr.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    AnvilRenderer::transitionImageLayout(inCmd, gBuffer.worldPosition.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void SponzaDeferred::recordLightingPass(VkCommandBuffer inCmd, Swapchain& inSwapchain)
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

    sponzaScene.updateGPUBuffer();

    if (UI::RenderDebugMenu(sponzaScene.data.debugViewMode))
    {
        sponzaScene.setGPUSceneData(sponzaScene.data);
        sponzaScene.updateGPUBuffer();
    }

    const float aspect = static_cast<float>(inSwapchain.swapchainExtent.width) /
                         static_cast<float>(inSwapchain.swapchainExtent.height);

    const glm::mat4 projection = camera.getProjectionMatrix(aspect);
    const glm::mat4 view = camera.getViewMatrix();

    UI::RenderWorldAxes(view);

    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_Light.pipeline);
    vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material_Light.materialPipelineLayout, 0, 1, &sceneLightingSet.descriptorSet, 0, nullptr);

    PushConstants pc{};
    pc.camera = glm::vec4(camera.position, 1.0f);
    vkCmdPushConstants(inCmd, material_Light.materialPipelineLayout, material_Light.pushConstantStages, 0, sizeof(PushConstants), &pc);

    // Draw 3 vertices to generate the fullscreen triangle
    vkCmdDraw(inCmd, 3, 1, 0, 0);

    AnvilRenderer::engineStats.drawCalls++;
    AnvilRenderer::engineStats.primitiveCount += 3;
}

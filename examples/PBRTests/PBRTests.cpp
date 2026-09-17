// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "PBRTests.h"
#include <iostream>
#include "Console.h"
#include "UIElements.h"

void PBRTests::initializeProject(VulkanContext& inContext, Swapchain& inSwapchain, AnvilRenderer& inRenderer)
{
    pContext = &inContext;
    pSwapchain = &inSwapchain;
    pRenderer = &inRenderer;

    gBuffer.create(*pContext, inSwapchain.swapchainExtent);
    pbrScene.createScene(*pContext);

    shaderCompiler.initializeShaderCompiler();
    shaderCompiler.addSearchPath(PROJECT_DIR);

    loadPipelines();

    sceneManager.discoverScenes(PROJECT_DIR "/scenes");
    if (!sceneManager.availableScenes.empty())
    {
        sceneManager.loadScene(0, *pContext, material_Geo, camera, pbrScene);
    }
}

void PBRTests::cleanupProject()
{
    if (pContext)
    {
        vkDeviceWaitIdle(pContext->device);

        gBuffer.destroy();
        sceneManager.gpuModel.destroyGPUModel();

        material_Geo.destroyMaterial();
        shaderProgram_Geo.destroyProgram();
        pipeline_Geo.destroy(pContext);

        material_Light.destroyMaterial();
        shaderProgram_Light.destroyProgram();
        pipeline_Light.destroy(pContext);

        shaderCompiler.shutdownShaderCompiler();
    }
}

bool PBRTests::loadPipelines(std::string* outErrorMessage)
{
    shaderCompiler.resetSession();

    if (!loadGeometryPipeline(outErrorMessage)) return false;

    // Refresh the GPU model on the active scene using the new geometry material
    if (sceneManager.activeSceneIndex >= 0)
    {
        sceneManager.reloadActiveScene(*pContext, material_Geo, camera, pbrScene);
    }

    if (!loadLightingPipeline(outErrorMessage)) return false;

    sceneLightingSet = material_Light.allocateSet(0);
    sceneLightingSet.bindTexture("gAlbedo", gBuffer.albedo);
    sceneLightingSet.bindTexture("gNormal", gBuffer.normal);
    sceneLightingSet.bindTexture("gPBR", gBuffer.pbr);
    sceneLightingSet.bindTexture("gWorldPosition", gBuffer.worldPosition);
    sceneLightingSet.bindUniformBuffer("sceneBuffer", pbrScene.sceneUBO);
    sceneLightingSet.updateDescriptorSets();

    return true;
}

bool PBRTests::loadGeometryPipeline(std::string* outErrorMessage)
{
    AnvilShaders::ShaderCompileRequest v_req{"PBRGeometry", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest f_req{"PBRGeometry", "fragmentMain", AnvilShaders::ST_Fragment};

    ShaderProgram new_program;
    if (!new_program.buildProgram(*pContext, shaderCompiler, v_req, f_req, outErrorMessage)) return false;

    pipeline_Geo.destroy(pContext);
    material_Geo.destroyMaterial();
    shaderProgram_Geo.destroyProgram();

    shaderProgram_Geo = std::move(new_program);
    material_Geo.buildMaterialFromProgram(*pContext, shaderProgram_Geo);

    std::vector<VkFormat> attachments = {
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R16G16B16A16_SFLOAT
    };

    auto bindings = {GPUMesh::GetBindingDescription()};
    auto attributes = GPUMesh::GetAttributeDescriptions({POSITION, NORMAL, TANGENT, UV });

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

    return true;
}

bool PBRTests::loadLightingPipeline(std::string* outErrorMessage)
{
    AnvilShaders::ShaderCompileRequest v_req{"PBRLighting", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest f_req{"PBRLighting", "fragmentMain", AnvilShaders::ST_Fragment};

    ShaderProgram new_program;
    if (!new_program.buildProgram(*pContext, shaderCompiler, v_req, f_req, outErrorMessage)) return false;

    pipeline_Light.destroy(pContext);
    material_Light.destroyMaterial();
    shaderProgram_Light.destroyProgram();

    shaderProgram_Light = std::move(new_program);
    material_Light.buildMaterialFromProgram(*pContext, shaderProgram_Light);

    PipelineBuilder builder;
    pipeline_Light = builder.setShaders(material_Light.getVertexShader(), material_Light.getFragmentShader())
        .setVertexInput({}, {})
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

void PBRTests::recordGeometryPass(VkCommandBuffer inCmd, const Swapchain& inSwapchain)
{
    if (gBuffer.currentExtent.width != inSwapchain.swapchainExtent.width || gBuffer.currentExtent.height != inSwapchain.swapchainExtent.height)
    {
        vkDeviceWaitIdle(pContext->device);
        gBuffer.create(*pContext, inSwapchain.swapchainExtent);
        sceneLightingSet.bindTexture("gAlbedo", gBuffer.albedo);
        sceneLightingSet.bindTexture("gNormal", gBuffer.normal);
        sceneLightingSet.bindTexture("gPBR", gBuffer.pbr);
        sceneLightingSet.bindTexture("gWorldPosition", gBuffer.worldPosition);
        sceneLightingSet.updateDescriptorSets();
    }

    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    camera.updateCamera(deltaTime);
    sceneManager.gpuModel.updateTransforms(sceneManager.cpuModel);

    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.albedo.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.normal.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.pbr.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.worldPosition.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.depth.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

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
    AnvilRenderer::SetViewportScissor(inCmd, inSwapchain);

    pRenderer->drawModel(inCmd, sceneManager.gpuModel, camera, pipeline_Geo.pipeline, material_Geo.materialPipelineLayout, VK_NULL_HANDLE, true);

    vkCmdEndRendering(inCmd);

    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.albedo.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.normal.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.pbr.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.worldPosition.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void PBRTests::recordLightingPass(VkCommandBuffer inCmd, Swapchain& inSwapchain)
{
    AnvilRenderer::SetViewportScissor(inCmd, inSwapchain);
    pbrScene.updateGPUBuffer();
    UI::RenderWorldAxes(camera.getViewMatrix());

    uint32_t debugMode = static_cast<uint32_t>(Console::GetCVarInt("r.debugmode"));
    int activeSceneIdx = sceneManager.activeSceneIndex;
    uint32_t selectedSceneIdx = 0;

    if (UI::DrawDebugMenu(debugMode, sceneManager.availableScenes, activeSceneIdx, selectedSceneIdx))
    {
        if (static_cast<int>(selectedSceneIdx) != activeSceneIdx)
        {
            sceneManager.loadScene(selectedSceneIdx, *pContext, material_Geo, camera, pbrScene);
        }
    }
    Console::SetCVarInt("r.debugmode", static_cast<int>(debugMode));

    if (DebugPass::isForwardMode(debugMode))
    {
        pRenderer->drawModel(inCmd, sceneManager.gpuModel, camera, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, false);
    }
    else
    {
        pRenderer->drawDeferredLighting(inCmd, gBuffer, camera, pipeline_Light.pipeline, material_Light.materialPipelineLayout, sceneLightingSet.descriptorSet);
    }
}
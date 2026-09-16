// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "SponzaDeferred.h"

#include <iostream>

#include "AnvilRenderer.h"
#include "Console.h"
#include "UIElements.h"

void SponzaDeferred::initializeProject(VulkanContext& inContext, Swapchain& inSwapchain, AnvilRenderer& inRenderer)
{
    std::cout << "Initialize project" << std::endl;
    pContext = &inContext;
    pSwapchain = &inSwapchain;
    pRenderer = &inRenderer;

    // Adjust camera to look down the main hall of Sponza
    camera.position = glm::vec3(0.0f, 2.0f, 0.0f);
    camera.cameraSpeed = 15.0f;

    const char* modelPath = ASSETS_DIR "/models/Sponza/glTF/Sponza.gltf";
    cpuModel.loadGLTF(modelPath);

    gBuffer.create(*pContext, inSwapchain.swapchainExtent);

    GlobalSceneData scene_data{};
    scene_data.lightDirection = glm::vec4(-0.2f, -1.0f, -0.2f, 0.0f);
    scene_data.lightColor = glm::vec4(0.5f, 0.4f, 0.2f, 1.0f);
    scene_data.ambientColor = glm::vec4(0.1f, 0.1f, 0.07f, 1.0f);

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

    std::cout << "Finished Loading Lighting Pipeline." << std::endl;

    return true;
}

void SponzaDeferred::recordGeometryPass(VkCommandBuffer inCmd, const Swapchain& inSwapchain)
{
    // Handle Window Resize
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

    // TODO: this delta time implementation needs some work
    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    camera.updateCamera(deltaTime);
    gpuModel.updateTransforms(cpuModel);

    // Transition G-Buffer to Attachment Optimal
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.albedo.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.normal.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.pbr.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.worldPosition.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.depth.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

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
    AnvilRenderer::SetViewportScissor(inCmd, inSwapchain);

    pRenderer->drawModel(inCmd, gpuModel, camera, pipeline_Geo.pipeline, material_Geo.materialPipelineLayout, VK_NULL_HANDLE, true);

    vkCmdEndRendering(inCmd);

    // Transition G-Buffer to Shader Read
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.albedo.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.normal.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.pbr.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    AnvilRenderer::TransitionImageLayout(inCmd, gBuffer.worldPosition.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void SponzaDeferred::recordLightingPass(VkCommandBuffer inCmd, Swapchain& inSwapchain)
{
    AnvilRenderer::SetViewportScissor(inCmd, inSwapchain);
    sponzaScene.updateGPUBuffer();
    UI::RenderWorldAxes(camera.getViewMatrix());

    uint32_t debugMode = static_cast<uint32_t>(Console::GetCVarInt("r.debugmode"));
    if (UI::DrawDebugMenu(debugMode))
    {
        Console::SetCVarInt("r.debugmode", static_cast<int>(debugMode));
    }

    if (DebugPass::isForwardMode(debugMode))
    {
        // Pass `false` because we are drawing Forward directly to the Swapchain
        pRenderer->drawModel(inCmd, gpuModel, camera, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, false);
    }
    else
    {
        pRenderer->drawDeferredLighting(inCmd, gBuffer, camera, pipeline_Light.pipeline, material_Light.materialPipelineLayout, sceneLightingSet.descriptorSet);
    }
}

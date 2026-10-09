// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "RenderGraphStub.h"

#include "Console.h"
#include "PushConstants.h"
#include "RenderGraph.h"
#include "UIElements.h"
#include "VulkanUtilities.h"

void RenderGraphApp::initialize()
{
    CreateInfo info{};
    info.width = 1280;
    info.height = 720;
    info.title = "RenderGraphTests";

    app.initialize(info);

    gBuffer.create(app.getContext(), app.getSwapchain().swapchainExtent);
    scene.createScene(app.getContext());
    sceneManager.discoverScenes(PROJECT_DIR "/scenes");

    std::string compilation_errors;
    if (!buildPipelines(&compilation_errors))
    {
        LOG_FATAL("{}", compilation_errors);
        FATAL(false, "Shader Compilation Failed.");
    }

    app.addShaderReloadCallback([&](std::string* outError)
    {
        return buildPipelines(outError);
    });

    if (sceneManager.hasScenes())
    {
        sceneManager.loadScene(0, app.getContext(), gBufferMaterial, camera, scene);
    }
}

void RenderGraphApp::cleanup()
{
    if (app.getContext().device)
    {
        vkDeviceWaitIdle(app.getContext().device);

        gBuffer.destroy();
        sceneManager.gpuModel.destroyGPUModel();

        gBufferPipeline.destroy(&app.getContext());
        gBufferMaterial.destroyMaterial();
        gBufferProgram.destroyProgram();

        deferredLightingPipeline.destroy(&app.getContext());
        deferredLightingMaterial.destroyMaterial();
        deferredLightingProgram.destroyProgram();

        app.shutdown();
    }
}

bool RenderGraphApp::buildPipelines(std::string* outErrorMessage)
{
    bool b_success = buildGBufferPipeline(outErrorMessage);
    b_success = buildLightingPipeline(outErrorMessage) && b_success;

    if (b_success && sceneManager.activeSceneIndex >= 0)
    {
        sceneManager.reloadActiveScene(app.getContext(), gBufferMaterial, camera, scene);
    }

    return b_success;
}

bool RenderGraphApp::buildGBufferPipeline(std::string* outErrorMessage)
{
    // G-Buffer Geometry Pipeline
    Shaders::ShaderCompileRequest v_req{"Geometry", "vertexMain", Shaders::ST_Vertex};
    Shaders::ShaderCompileRequest f_req{"Geometry", "fragmentMain", Shaders::ST_Fragment};

    // Extra step for hot reloading support
    ShaderProgram temp_program;
    bool result = temp_program.buildProgram(app.getContext(), app.getShaderCompiler(), v_req, f_req, outErrorMessage);
    if (result == false)
    {
        return false;
    }

    // Compilation was successfull
    // Create pipeline and material
    gBufferPipeline.destroy(&app.getContext());
    gBufferMaterial.destroyMaterial();
    gBufferProgram.destroyProgram();

    gBufferProgram = std::move(temp_program);
    gBufferMaterial.buildMaterialFromProgram(app.getContext(), gBufferProgram);

    auto attributes = GPUMesh::GetAttributeDescriptions(
    {POSITION, NORMAL, TANGENT, UV}
    );
    auto bindings = GPUMesh::GetBindingDescriptions();

    std::vector attachments = {
        Format::RGBA8_UNORM,
        Format::RGBA16_SFLOAT,
        Format::RGBA8_UNORM,
        Format::RGBA16_SFLOAT,
    };

    PipelineBuilder builder;
    builder.setShaders(gBufferMaterial)
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormats(attachments)
        .setDepthAttachmentFormat(Format::D32_SFLOAT)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending();

    gBufferPipeline = builder.buildPipeline(app.getContext().device, gBufferMaterial.materialPipelineLayout DNAME("GBufferGeometryPipeline"));

    return true;
}

bool RenderGraphApp::buildLightingPipeline(std::string* outErrorMessage)
{
    // Lighting Pipeline
    Shaders::ShaderCompileRequest v_req{"Lighting", "vertexMain", Shaders::ST_Vertex};
    Shaders::ShaderCompileRequest f_req{"Lighting", "fragmentMain", Shaders::ST_Fragment};

    // Extra step for hot reloading support
    ShaderProgram temp_program;
    bool result = temp_program.buildProgram(app.getContext(), app.getShaderCompiler(), v_req, f_req, outErrorMessage);
    if (result == false)
    {
        return false;
    }

    // Compilation was successfull
    // Create pipeline and material
    deferredLightingProgram = std::move(temp_program);
    deferredLightingMaterial.buildMaterialFromProgram(app.getContext(), deferredLightingProgram);

    PipelineBuilder builder;
    builder.setShaders(deferredLightingMaterial)
        .setVertexInput({}, {})
        .setColorAttachmentFormats({app.getSwapchain().swapchainFormat})
        .setDepthAttachmentFormat(app.getSwapchain().depthFormat)
        .enableDepthTest(false, VK_COMPARE_OP_ALWAYS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disableBlending();

    deferredLightingPipeline = builder.buildPipeline(app.getContext().device, deferredLightingMaterial.materialPipelineLayout DNAME("DeferredLightingPipeline"));

    // Allocate Set 0 for Deferred Lighting
    lightingSet0 = deferredLightingMaterial.allocateSet(0);
    lightingSet0.bindUniformBuffer("sceneBuffer", scene.sceneUBO);
    lightingSet0.bindTexture("gAlbedo", gBuffer.albedo);
    lightingSet0.bindTexture("gNormal", gBuffer.normal);
    lightingSet0.bindTexture("gPBR", gBuffer.pbr);
    lightingSet0.bindTexture("gWorldPosition", gBuffer.worldPosition);
    lightingSet0.updateDescriptorSets();

    return true;
}

void RenderGraphApp::run()
{
    RenderHooks hooks;
    hooks.onRecordFrame = [this](VkCommandBuffer cmd)
    {
        camera.updateCamera(Application::engineStats.frameTime/1000.f);
        recordRenderGraph(cmd);
    };
    hooks.onDrawUI = [&]()
    {
        changeScene = UI::DrawScenesMenu(sceneManager);
        UI::RenderWorldAxes(camera.getViewMatrix());
    };
    app.run(hooks);
}

void RenderGraphApp::recordRenderGraph(VkCommandBuffer cmd)
{
    if (changeScene)
    {
        // Because the Scene Menu UI already changes the Active scene in SceneConfig
        sceneManager.reloadActiveScene(app.getContext(), gBufferMaterial, camera, scene);
    }

    SCOPE_CPU;
    SCOPE_GPU(app.getRenderContext().tracyVkCtx, cmd, "RenderGraph");

    // Handle viewport/GBuffer resizing dynamically
    VkExtent2D current_extent = app.getSwapchain().swapchainExtent;
    if (gBuffer.shouldRecreate(current_extent))
    {
        // Safely wait for the GPU to idle before rebuilding the GBuffer
        // and updating descriptors while the previous frame is still in flight!
        vkDeviceWaitIdle(app.getContext().device);

        gBuffer.create(app.getContext(), current_extent);
        app.getDebugPass().cachedGBufferView = VK_NULL_HANDLE;

        // Rebind scene UBO along with new textures to ensure it never gets dropped
        lightingSet0.bindUniformBuffer("sceneBuffer", scene.sceneUBO);

        lightingSet0.bindTexture("gAlbedo", gBuffer.albedo);
        lightingSet0.bindTexture("gNormal", gBuffer.normal);
        lightingSet0.bindTexture("gPBR", gBuffer.pbr);
        lightingSet0.bindTexture("gWorldPosition", gBuffer.worldPosition);
        lightingSet0.updateDescriptorSets();
    }

    camera.updateCamera(Application::engineStats.frameTime/1000.f);
    sceneManager.gpuModel.updateTransforms(sceneManager.cpuModel);

    const glm::vec4 grey(0.1f, 0.1f, 0.1f, 1.0f);
    RenderGraph graph;

    auto gbufferPass = graph.addPass("GBuffer Geometry Pass")
        .writeColor(gBuffer.albedo, LoadOp::Clear, grey)
        .writeColor(gBuffer.normal, LoadOp::Clear, grey)
        .writeColor(gBuffer.pbr, LoadOp::Clear, grey)
        .writeColor(gBuffer.worldPosition, LoadOp::Clear, grey)
        .writeDepth(gBuffer.depth, LoadOp::Clear, 1.0f);

    gbufferPass.execute([this](VkCommandBuffer passCmd)
    {
        SCOPE_GPU(app.getRenderContext().tracyVkCtx, passCmd, "GBuffer Geometry Pass");
        drawGBufferGeometry(passCmd);
    });

    auto lightingPass = graph.addPass("Deferred Lighting Pass")
        .read(gBuffer.albedo)
        .read(gBuffer.normal)
        .read(gBuffer.pbr)
        .read(gBuffer.worldPosition)
        .read(gBuffer.depth)
        .writeSwapchain(app.getSwapchain(), app.getRenderContext().imageIndex, LoadOp::Clear, glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));

    lightingPass.execute([this](VkCommandBuffer passCmd)
    {
        SCOPE_GPU(app.getRenderContext().tracyVkCtx, passCmd, "Deferred Lighting Pass");
        drawDeferredLighting(passCmd);
    });

    graph.execute(cmd);
}

void RenderGraphApp::drawGBufferGeometry(VkCommandBuffer cmd)
{
    SCOPE_CPU;

    uint32_t debug_mode = static_cast<uint32_t>(Console::GetCVarInt("r.debugmode"));
    bool is_forward_debug = DebugPass::isForwardMode(debug_mode);
    bool is_debug = static_cast<DebugMode>(debug_mode) > DebugMode::None;
    bool is_frozen = Console::GetCVarBool("r.freezerendering");
    bool is_culling = Console::GetCVarBool("r.frustumculling");

    if (is_forward_debug) return;

    bool use_debug_pipeline = is_debug;
    VkPipeline active_pipeline = use_debug_pipeline ? app.getDebugPass().getForwardPipeline(debug_mode).pipeline : gBufferPipeline.pipeline;
    VkPipelineLayout active_layout = use_debug_pipeline ? app.getDebugPass().getForwardLayout() : gBufferMaterial.materialPipelineLayout;

    if (active_pipeline == VK_NULL_HANDLE) return;

    VulkanUtils::SetViewportScissor(cmd, app.getSwapchain());

    const float aspect = static_cast<float>(app.getSwapchain().swapchainExtent.width) / static_cast<float>(app.getSwapchain().swapchainExtent.height);
    const glm::mat4 view_projection = camera.getProjectionMatrix(aspect) * camera.getViewMatrix();

    static glm::mat4 frozen_vp = view_projection;
    static bool was_frozen = false;
    if (is_frozen && !was_frozen) frozen_vp = view_projection;
    was_frozen = is_frozen;

    Frustum camera_frustum{};
    camera_frustum.extractPlanes(is_frozen ? frozen_vp : view_projection);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, active_pipeline);

    VkDeviceSize offset = 0;
    GPUModel& gpuModel = sceneManager.gpuModel;
    for (Index32 i = 0; i < gpuModel.drawItems.size(); ++i)
    {
        const GPUModelDrawItem& draw_item = gpuModel.drawItems[i];

        if (draw_item.gpuMeshIndex >= gpuModel.gpuMeshes.size()) continue;

        if (is_culling)
        {
            SCOPE_CPU_NAME("Frustum Culling");
            glm::vec3 center = draw_item.localBounds.getCenter();
            glm::vec3 extents = draw_item.localBounds.getExtents();
            glm::vec3 worldCenter = glm::vec3(draw_item.worldMatrix * glm::vec4(center, 1.0f));
            glm::mat3 absModel = glm::mat3(
                glm::abs(draw_item.worldMatrix[0]),
                glm::abs(draw_item.worldMatrix[1]),
                glm::abs(draw_item.worldMatrix[2])
            );
            glm::vec3 worldExtents = absModel * extents;

            AABB worldAABB{.min = worldCenter - worldExtents, .max = worldCenter + worldExtents};
            if (!camera_frustum.contains(worldAABB)) continue;
        }

        std::vector<VkDescriptorSet> sets;
        uint32_t first_set = 1;

        VkDescriptorSet matSet = VK_NULL_HANDLE;
        if (draw_item.gpuMaterialIndex >= 0 && draw_item.gpuMaterialIndex < static_cast<int>(gpuModel.gpuMaterials.size()))
        {
            matSet = gpuModel.gpuMaterials[draw_item.gpuMaterialIndex].instance.descriptorSet;
        }
        else if (!gpuModel.gpuMaterials.empty())
        {
            matSet = gpuModel.gpuMaterials[0].instance.descriptorSet;
        }

        if (gpuModel.modelSet.descriptorSet != VK_NULL_HANDLE) sets.push_back(gpuModel.modelSet.descriptorSet);
        if (matSet != VK_NULL_HANDLE) sets.push_back(matSet);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, active_layout, first_set, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

        PushConstants constants{};
        constants.viewProjection = view_projection;
        constants.cameraPosition = glm::vec4(camera.position, 1.0f);
        constants.objectIndex = i;
        constants.debugMode = static_cast<DebugMode>(debug_mode);
        vkCmdPushConstants(cmd, active_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);

        const GPUMesh& mesh = gpuModel.gpuMeshes[draw_item.gpuMeshIndex];
        vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);

        Application::engineStats.drawCalls++;
        Application::engineStats.primitiveCount += (mesh.indexCount / 3);
    }
}

void RenderGraphApp::drawDeferredLighting(VkCommandBuffer cmd)
{
    SCOPE_CPU;

    uint32_t debug_mode = static_cast<uint32_t>(Console::GetCVarInt("r.debugmode"));

    if (static_cast<DebugMode>(debug_mode) == DebugMode::None)
    {
        if (deferredLightingPipeline.pipeline == VK_NULL_HANDLE) return;

        VulkanUtils::SetViewportScissor(cmd, app.getSwapchain());

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLightingPipeline.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLightingMaterial.materialPipelineLayout, 0, 1, &lightingSet0.descriptorSet, 0, nullptr);

        PushConstants pc{};
        pc.cameraPosition = glm::vec4(camera.position, 1.0f);
        pc.debugMode = DebugMode::None;
        vkCmdPushConstants(cmd, deferredLightingMaterial.materialPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);

        vkCmdDraw(cmd, 3, 1, 0, 0); // Render fullscreen quad
    }
    else if (DebugPass::isDeferredMode(debug_mode))
    {
        VulkanUtils::SetViewportScissor(cmd, app.getSwapchain());
        app.getDebugPass().drawDeferredResolve(cmd, gBuffer, static_cast<DebugMode>(debug_mode), glm::vec4(camera.position, 1.0f));
    }
}



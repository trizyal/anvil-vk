// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "RenderGraphStub.h"

#include "RenderGraph.h"

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
    app.run(hooks);
}

void RenderGraphApp::recordRenderGraph(VkCommandBuffer cmd)
{
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

    const glm::vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
    RenderGraph graph;

    auto gbufferPass = graph.addPass("GBuffer Geometry Pass")
        .writeColor(gBuffer.albedo, LoadOp::Clear, black)
        .writeColor(gBuffer.normal, LoadOp::Clear, black)
        .writeColor(gBuffer.pbr, LoadOp::Clear, black)
        .writeColor(gBuffer.worldPosition, LoadOp::Clear, black)
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

}

void RenderGraphApp::drawDeferredLighting(VkCommandBuffer cmd)
{

}



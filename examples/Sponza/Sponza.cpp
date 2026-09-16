// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Sponza.h"

#include <iostream>

#include "AnvilRenderer.h"
#include "Console.h"
#include "UIElements.h"

void Sponza::initializeProject(VulkanContext& inContext, Swapchain& inSwapchain, AnvilRenderer& inRenderer)
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

    GlobalSceneData scene_data{};
    scene_data.lightDirection = glm::vec4(-0.2f, -1.0f, -0.2f, 0.0f);
    scene_data.lightColor = glm::vec4(1.5f, 1.4f, 1.2f, 1.0f);
    scene_data.ambientColor = glm::vec4(0.2f, 0.25f, 0.3f, 1.0f);

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
        .setColorAttachmentFormats({pSwapchain->swapchainFormat})
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
    AnvilRenderer::SetViewportScissor(inCmd, inSwapchain);

    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
    lastFrameTime = currentTime;

    camera.updateCamera(deltaTime);
    gpuModel.updateTransforms(cpuModel);
    sponzaScene.updateGPUBuffer();

    uint32_t cvarDebugMode = static_cast<uint32_t>(Console::GetCVarInt("r.debugmode"));
    if (UI::DrawDebugMenu(cvarDebugMode))
    {
        Console::SetCVarInt("r.debugmode", static_cast<int>(cvarDebugMode));
    }

    // The Engine handles Culling, Pipeline Overrides, and the Draw Loop!
    // We pass `false` at the end to indicate this is a Forward-Only project.
    pRenderer->drawModel(inCmd, gpuModel, camera, pipeline.pipeline, sponzaMaterial.materialPipelineLayout, globalSet.descriptorSet, false);
}

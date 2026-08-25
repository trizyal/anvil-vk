// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Sponza.h"

#include <iostream>

void Sponza::initializeProject(VulkanContext& inContext, Swapchain& inSwapchain)
{
    std::cout << "Initialize project" << std::endl;
    pContext = &inContext;
    pSwapchain = &inSwapchain;

    // Adjust camera to look down the main hall of Sponza
    camera.position = glm::vec3(0.0f, 2.0f, 0.0f);
    camera.cameraSpeed = 15.0f;

    const char* modelPath = PROJECT_DIR "/Sponza/glTF/Sponza.gltf";
    cpuModel.loadGLTF(modelPath);

    DirectionalLighting sceneLighting{};
    sceneLighting.lightDirection = glm::vec4(-0.2f, -1.0f, -0.2f, 0.0f);
    sceneLighting.lightColor = glm::vec4(1.5f, 1.4f, 1.2f, 1.0f);
    sceneLighting.ambientColor = glm::vec4(0.2f, 0.25f, 0.3f, 1.0f);

    sponzaScene.createScene(*pContext);
    sponzaScene.setGPUSceneData(sceneLighting);
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

}

void Sponza::loadPipeline()
{
    std::cout << "Loading Pipeline." << std::endl;
    shaderCompiler.resetSession();
    if (pipeline.pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(pContext->device, pipeline.pipeline, nullptr);
        pipeline.pipeline = VK_NULL_HANDLE;
        sponzaMaterial.destroyMaterial();
    }

    AnvilShaders::ShaderCompileRequest vReq{"Sponza", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"Sponza", "fragmentMain", AnvilShaders::ST_Fragment};

    sponzaMaterial.buildMaterial(*pContext, shaderCompiler, vReq, fReq);

    // Setup Set 0
    globalSet = sponzaMaterial.allocateSet(0);
    globalSet.bindUniformBuffer("sceneBuffer", sponzaScene.sceneUBO);
    globalSet.updateDescriptorSets();

    auto attributes = GPUMesh::GetAttributeDescriptions5();
    attributes.resize(3);

    std::vector<VkVertexInputBindingDescription> bindings = {GPUMesh::GetBindingDescription()};

    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(sponzaMaterial.vertexShader.get(), sponzaMaterial.fragmentShader.get())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormat(pSwapchain->swapchainFormat)
        .setDepthAttachmentFormat(pSwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE) // NO CULLING FOR CURTAINS
        .disableBlending()
        .buildPipeline(pContext->device, sponzaMaterial.materialPipelineLayout, "SponzaPipeline");

    gpuModel.createGPUModel(*pContext, cpuModel, sponzaMaterial);

    std::cout << "Pipeline loading completed." << std::endl;
}

void Sponza::recordCommands(VkCommandBuffer inCmd, Swapchain& inSwapchain)
{

}

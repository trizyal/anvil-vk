// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "BoxAnimated.h"

void BoxAnimated::initializeProject(VulkanContext& inContext, VulkanSwapchain& inSwapchain)
{
    pContext = &inContext;
    pSwapchain = &inSwapchain;

    const char* modelPath = PROJECT_DIR "/BoxAnimated/glTF/BoxAnimated.gltf";
    cpuModel.loadGLTF(modelPath);

    // Setup initial light values
    GPUSceneData sceneLighting{};
    sceneLighting.lightDirection = glm::vec4(-1.0f, -1.0f, -0.5f, 0.0f); // Sunlight pointing down-left
    sceneLighting.lightColor = glm::vec4(1.0f, 0.95f, 0.8f, 2.0f); // Warm sunlight, intensity = 2.0
    sceneLighting.ambientColor = glm::vec4(0.08f, 0.1f, 0.15f, 1.0f); // Cool blue sky ambient

    boxScene.createScene(*pContext);
    boxScene.setGPUSceneData(sceneLighting);
    boxScene.updateGPUBuffer();

    // Initialize shader compiler
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);
    loadPipeline();
}

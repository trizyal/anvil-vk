// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_HELLOCUBE_H
#define EXAMPLE_HELLOCUBE_H

#include <glm/glm.hpp>
#include <vector>

#include "GPUBuffer.h"
#include "Camera.h"
#include "AnvilMaterial.h"
#include "VulkanContext.h"
#include "PipelineBuilder.h"
#include "Scene.h"
#include "ShaderCompiler.h"
#include "Swapchain.h"
#include "ShaderProgram.h"

// The data we push to the shader every frame
struct ProjectPushConstants
{
    glm::mat4 renderMatrix;
};

// Hardcoded Vertex definition for this specific example
struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
};

class HelloCube
{
private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;
    ShaderCompiler shaderCompiler;

    AnvilPipeline pipeline = {};
    Camera camera;
    Scene myScene;

    ShaderProgram myProgram; // Explicitly manage program layout
    AnvilMaterial myMaterial;
    MaterialInstance globalSet;

    GPUBuffer vertexBuffer;
    GPUBuffer indexBuffer;

public:
    void initializeProject(VulkanContext& inAnvilContext, Swapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, Swapchain &inAnvilSwapchain);

    void loadPipeline();

private:
    void createBuffers();
};

#endif //EXAMPLE_HELLOCUBE_H
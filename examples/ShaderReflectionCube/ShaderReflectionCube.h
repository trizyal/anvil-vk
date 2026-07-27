// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_SHADERREFLECTIONCUBE_H
#define EXAMPLE_SHADERREFLECTIONCUBE_H

#include <glm/glm.hpp>

#include "AnvilCamera.h"
#include "AnvilMeshBuffer.h"
#include "AnvilVulkanContext.h"
#include "AnvilShaderModule.h"
#include "AnvilPipeline.h"
#include "AnvilShaderCompiler.h"
#include "AnvilSwapchain.h"
#include "AnvilTextureLoader.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix;
};

class ShaderReflectionCube
{
private:
    AnvilVulkanContext* ptrAContext = nullptr;
    AnvilSwapchain* ptrASwapchain = nullptr;

    AnvilShaderModule vertexShader;
    AnvilShaderModule fragmentShader;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    AnvilPipeline pipeline = {};

    AnvilMeshBuffer meshBuffer;

    AnvilShaderCompiler shaderCompiler;

    AnvilCamera camera;

    // Things for textures
    AnvilTexture myTexture;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

public:
    void initializeProject(AnvilVulkanContext& inAnvilContext, AnvilSwapchain& inAnvilSwapchain);
    void cleanupProject();

    // Function that records commands to trigger in AnvilRenderer
    void recordCommands(VkCommandBuffer inCmd, AnvilSwapchain &inAnvilSwapchain);

    void loadPipeline();

    void setupDescriptors();
};


#endif //EXAMPLE_SHADERREFLECTIONCUBE_H

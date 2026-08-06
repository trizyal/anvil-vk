// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef EXAMPLE_TEXTURECUBE_H
#define EXAMPLE_TEXTURECUBE_H

#include <glm/glm.hpp>

#include "AnvilCamera.h"
#include "AnvilMeshBuffer.h"
#include "AnvilVulkanContext.h"
#include "ShaderModule.h"
#include "PipelineBuilder.h"
#include "AnvilShaderCompiler.h"
#include "AnvilSwapchain.h"
#include "AnvilTextureLoader.h"

// The data we push to the shader every frame (Must be <= 128 bytes)
struct PushConstants
{
    glm::mat4 renderMatrix;
};

class TextureCube
{
private:
    AnvilVulkanContext* ptrAContext = nullptr;
    AnvilSwapchain* ptrASwapchain = nullptr;

    ShaderModule vertexShader;
    ShaderModule fragmentShader;
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


#endif //EXAMPLE_TEXTURECUBE_H

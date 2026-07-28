// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_MATERIAL_H
#define ANVIL_VK_MATERIAL_H

#include <string>

#include <volk.h>
#include <slang.h>
#include <slang-com-ptr.h>

#include "AnvilBuffer.h"
#include "AnvilShaderCompiler.h"
#include "AnvilShaderModule.h"
#include "AnvilTextureLoader.h"
#include "AnvilVulkanContext.h"

struct ShaderBinding
{
    uint32_t set;
    uint32_t binding;
    VkDescriptorType descriptorType;
};

class AnvilMaterial
{
public:
    // The Material now owns its own shaders!
    AnvilShaderModule vertexShader;
    AnvilShaderModule fragmentShader;

    VkPipelineLayout materialPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout materialDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool materialDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet materialDescriptorSet = VK_NULL_HANDLE;

    VkShaderStageFlags pushConstantStages = 0;

private:
    AnvilVulkanContext* ptrAContext = nullptr;

    std::unordered_map<std::string, ShaderBinding> bindingMap;
    std::vector<VkWriteDescriptorSet> pendingWrites;
    std::vector<VkDescriptorImageInfo> pendingImageInfos;
    std::vector<VkDescriptorBufferInfo> pendingBufferInfos;

public:
    void buildMaterial(AnvilVulkanContext& inContext,
        AnvilShaderCompiler& inCompiler,
        const AnvilShaders::ShaderCompileRequest& inVertReq,
        const AnvilShaders::ShaderCompileRequest& inFragReq);

    void destroyMaterial() const;

    void bindTexture(const std::string& name, const AnvilTexture& inTexture);
    void bindUniformBuffer(const std::string& name, const AnvilBuffer& inBuffer);
    void updateDescriptorSets();

private:
    void reflectShader(slang::IComponentType* linkedProgram,
        VkShaderStageFlagBits stage,
        std::vector<VkDescriptorSetLayoutBinding>& layoutBindings,
        std::vector<VkPushConstantRange>& pushConstants);
};

#endif //ANVIL_VK_MATERIAL_H

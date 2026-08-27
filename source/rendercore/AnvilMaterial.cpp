// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilMaterial.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <ranges>
#include <unordered_map>

#include "DebugNames.h"
#include "VulkanResult.h"

void AnvilMaterial::buildMaterial(VulkanContext& inContext,
    ShaderCompiler& inCompiler,
    const AnvilShaders::ShaderCompileRequest& inVertReq,
    const AnvilShaders::ShaderCompileRequest& inFragReq)
{
    // Backwards compatibility wrapper
    legacyProgram = std::make_unique<ShaderProgram>();
    legacyProgram->buildProgram(inContext, inCompiler, inVertReq, inFragReq);

    buildMaterialFromProgram(inContext, *legacyProgram);
}

void AnvilMaterial::buildMaterialFromProgram(VulkanContext& inContext, const ShaderProgram& inProgram)
{
    pContext = &inContext;
    pActiveProgram = &inProgram;

    uint32_t max_set = 0;

    // ------------------
    // MERGE BINDINGS
    // ------------------
    // merged_set_bindings: A nested map used to de-duplicate the raw_reflected_bindings list.
    // - the outer map's key is the Descriptor `Set` Index
    // - the inner map's key is the `Binding` slot index within that set.
    // If a binding exists in multiple shader stages, we use this map to merge them
    // into a single binding that has a combined stageFlag.
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>> merged_set_bindings;

    for (const auto& b : inProgram.rawReflectedBindings)
    {
        max_set = std::max(max_set, b.setIndex);

        // If the binding slot already exists in this set, merge the stage flags via a bitwise OR.
        if (merged_set_bindings[b.setIndex].contains(b.bindingData.binding))
        {
            merged_set_bindings[b.setIndex][b.bindingData.binding].stageFlags |= b.bindingData.stageFlags;
        }
        else
        {
            merged_set_bindings[b.setIndex][b.bindingData.binding] = b.bindingData;
        }
    }

    descriptorSetLayouts.resize(max_set + 1, VK_NULL_HANDLE);
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (uint32_t set_index = 0; set_index <= max_set; ++set_index)
    {
        std::vector<VkDescriptorSetLayoutBinding> set_bindings;
        if (merged_set_bindings.contains(set_index))
        {
            // Extract the merged bindings from our map back into a flat vector for Vulkan
            for (const auto& binding_data : merged_set_bindings[set_index] | std::views::values)
            {
                set_bindings.push_back(binding_data);
                pool_sizes.push_back({.type = binding_data.descriptorType, .descriptorCount = binding_data.descriptorCount * 1000});
            }
        }

        VkDescriptorSetLayoutCreateInfo descriptor_layout_info{};
        descriptor_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_layout_info.bindingCount = static_cast<uint32_t>(set_bindings.size());
        descriptor_layout_info.pBindings = set_bindings.data();
        CHECK(vkCreateDescriptorSetLayout(pContext->device, &descriptor_layout_info, nullptr, &descriptorSetLayouts[set_index]));
        SET_DNAME_HERE(pContext->device, descriptorSetLayouts[set_index], VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, inProgram.name.c_str());
    }

    if (!pool_sizes.empty())
    {
        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = 1000; //_pool_sizes.size()?
        CHECK(vkCreateDescriptorPool(pContext->device, &pool_info, nullptr, &materialDescriptorPool));
        SET_DNAME_HERE(pContext->device, materialDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, inProgram.name.c_str());
    }

    // Push Constants and Pipeline Layout
    VkPushConstantRange merged_push_constant{};
    if (!inProgram.rawReflectedPushConstants.empty())
    {
        merged_push_constant = inProgram.rawReflectedPushConstants[0];
        for (size_t i = 1; i < inProgram.rawReflectedPushConstants.size(); ++i)
        {
            merged_push_constant.size = std::max(merged_push_constant.size, inProgram.rawReflectedPushConstants[i].size);
            merged_push_constant.stageFlags |= inProgram.rawReflectedPushConstants[i].stageFlags;
        }

        pushConstantStages = merged_push_constant.stageFlags;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipeline_layout_info.pSetLayouts = descriptorSetLayouts.data();
    if (merged_push_constant.size > 0)
    {
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &merged_push_constant;
    }
    CHECK(vkCreatePipelineLayout(pContext->device, &pipeline_layout_info, nullptr, &materialPipelineLayout));
    SET_DNAME_HERE(pContext->device, materialPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, inProgram.name.c_str());
}

MaterialInstance AnvilMaterial::createInstance() const
{
    return allocateSet(0);
}

MaterialInstance AnvilMaterial::allocateSet(const uint32_t setIndex) const
{
    MaterialInstance instance;
    instance.pContext = pContext;
    instance.pParentMaterial = this;

    if (materialDescriptorPool != VK_NULL_HANDLE && setIndex < descriptorSetLayouts.size())
    {
        instance.setIndex = setIndex;
        
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = materialDescriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &descriptorSetLayouts[setIndex];

        CHECK(vkAllocateDescriptorSets(pContext->device, &alloc_info, &instance.descriptorSet));
    }

    return instance;
}

bool AnvilMaterial::hasBinding(const std::string& name) const
{
    return pActiveProgram && pActiveProgram->bindingMap.contains(name);
}

ShaderBinding AnvilMaterial::getBinding(const std::string& name) const
{
    return pActiveProgram->bindingMap.at(name);

#if 0 // not sure which way is better yet
    const auto it = pActiveProgram->bindingMap.find(name);
    if (it == pActiveProgram->bindingMap.end())
    {
        throw std::runtime_error("AnvilMaterial binding does not exist: " + name);
    }
    return it->second;
#endif
}

bool AnvilMaterial::hasSet(uint32_t setIndex) const
{
    return setIndex < descriptorSetLayouts.size() && descriptorSetLayouts[setIndex] != VK_NULL_HANDLE;
}

void AnvilMaterial::destroyMaterial()
{
    if (pContext)
    {
        // Destroy legacy program if this material owns it (backwards compatibility)
        if (legacyProgram)
        {
            legacyProgram->destroyProgram();
            legacyProgram.reset();
        }

        if (materialDescriptorPool)
        {
            vkDestroyDescriptorPool(pContext->device, materialDescriptorPool, nullptr);
        }

        for (VkDescriptorSetLayout layout : descriptorSetLayouts)
        {
            if (layout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(pContext->device, layout, nullptr);
            }
        }

        descriptorSetLayouts.clear();
        if (materialPipelineLayout)
        {
            vkDestroyPipelineLayout(pContext->device, materialPipelineLayout, nullptr);
        }

        // Clear active program pointer
        pActiveProgram = nullptr;
        pContext = nullptr;
    }
}

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

void AnvilMaterial::reflectShader(slang::IComponentType* linkedProgram,
                                  const VkShaderStageFlagBits stage,
                                  std::vector<ReflectedBinding>& outLayoutBindings,
                                  std::vector<VkPushConstantRange>& outPushConstants)
{
    slang::ShaderReflection* reflection = linkedProgram->getLayout();

    if (!reflection)
    {
        std::cerr << "Failed to get Slang reflection layout!" << std::endl;
        return;
    }

    uint32_t param_count = reflection->getParameterCount();

    std::cout << "ParamCount received in AnvilMaterial: " << param_count << std::endl;

    for (uint32_t i = 0; i < param_count; i++)
    {
        slang::VariableLayoutReflection* var_layout = reflection->getParameterByIndex(i);
        slang::TypeLayoutReflection* type_layout = var_layout->getTypeLayout();

        const char* name = var_layout->getName();
        const slang::ParameterCategory category = var_layout->getCategory();

        if (category == slang::ParameterCategory::PushConstantBuffer)
        {
            VkPushConstantRange range{};
            range.stageFlags = stage;
            range.offset = static_cast<uint32_t>(var_layout->getOffset());

            // SLANG FIX: A ConstantBuffer<T> is a wrapper. We need the size of 'T' (the element).
            slang::TypeLayoutReflection* element_type = type_layout->getElementTypeLayout();
            if (element_type != nullptr) {
                range.size = static_cast<uint32_t>(element_type->getSize());
            } else {
                range.size = static_cast<uint32_t>(type_layout->getSize());
            }

            outPushConstants.push_back(range);

            std::cout << "Reflected Push Constant: " << name << " Size: " << range.size << "\n";
            continue;
        }

        if (category == slang::ParameterCategory::DescriptorTableSlot ||
            category == slang::ParameterCategory::Mixed)
        {
            VkDescriptorSetLayoutBinding layout_binding{};
            layout_binding.binding = static_cast<uint32_t>(var_layout->getBindingIndex());
            layout_binding.descriptorCount = 1;
            layout_binding.stageFlags = stage;

            slang::TypeReflection::Kind kind = type_layout->getKind();
            if (kind == slang::TypeReflection::Kind::Resource)
            {
                SlangResourceShape shape = type_layout->getResourceShape();

                // Had a bug here where all resources were being mapped as Images.
                if (shape == SLANG_STRUCTURED_BUFFER)
                {
                    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                }
                else
                {
                    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                }
            }
            else if (kind == slang::TypeReflection::Kind::ConstantBuffer)
            {
                layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
            else
            {
                continue;
            }

            ReflectedBinding reflected_binding{};
            reflected_binding.setIndex = var_layout->getBindingSpace();
            reflected_binding.bindingData = layout_binding;

            outLayoutBindings.push_back(reflected_binding);

            ShaderBinding shader_binding{};
            shader_binding.setIndex = var_layout->getBindingSpace();
            shader_binding.bindingIndex = layout_binding.binding;
            shader_binding.descriptorType = layout_binding.descriptorType;
            bindingMap[name] = shader_binding;
        }
    }
}

void AnvilMaterial::buildMaterial(VulkanContext& inContext,
    ShaderCompiler& inCompiler,
    const AnvilShaders::ShaderCompileRequest& inVertReq,
    const AnvilShaders::ShaderCompileRequest& inFragReq)
{
    pContext = &inContext;
    std::string material_debug_name = inVertReq.moduleName;

    // Compile Shaders internally
    const auto vertex_result = inCompiler.compileToSPIRV(inVertReq);
    const auto fragment_result = inCompiler.compileToSPIRV(inFragReq);

    // -----------------------
    // RAW REFLECTION DATA
    // -----------------------
    // raw_reflected_bindings: A flat list storing every individual descriptor binding discovered
    // in the shaders. If a resource (like a Scene UBO) is used in BOTH the Vertex and
    // Fragment shaders, it will be added to this list twice.
    std::vector<ReflectedBinding> raw_reflected_bindings;

    // raw_reflected_push_constants: A flat list of push constant ranges extracted from the shaders.
    // Like bindings, if multiple shader stages use push constants, we will get multiple
    // entries here that need to be combined later.
    std::vector<VkPushConstantRange> raw_reflected_push_constants;

    // We pass stage flags (e.g., VK_SHADER_STAGE_VERTEX_BIT) into the reflection parser.
    // This tags the discovered bindings so Vulkan knows EXACTLY which programmable stage
    // of the GPU pipeline (Vertex processing vs. Pixel processing) is allowed to read them.
    if (vertex_result.reflection)
    {
        reflectShader(vertex_result.reflection.get(),
            VK_SHADER_STAGE_VERTEX_BIT,
            raw_reflected_bindings,
            raw_reflected_push_constants);
    }

    if (fragment_result.reflection)
    {
        reflectShader(fragment_result.reflection.get(),
            VK_SHADER_STAGE_FRAGMENT_BIT,
            raw_reflected_bindings,
            raw_reflected_push_constants);
    }

    // Build Vulkan Shader Modules
    std::string debug_name = "MaterialVertexShader: " + inVertReq.moduleName;
    vertexShader.createShaderModule(*pContext, vertex_result DNAME(debug_name.c_str()));

    debug_name = "MaterialFragmentShader: " + inFragReq.moduleName;
    fragmentShader.createShaderModule(*pContext, fragment_result DNAME(debug_name.c_str()));

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

    for (const auto& b : raw_reflected_bindings)
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

        debug_name = "MaterialDescriptorSetLayout: " + material_debug_name;
        SET_DNAME_HERE(pContext->device, descriptorSetLayouts[set_index], VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, debug_name.c_str());
    }

    if (!pool_sizes.empty())
    {
        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = 1000; //_pool_sizes.size()?
        CHECK(vkCreateDescriptorPool(pContext->device, &pool_info, nullptr, &materialDescriptorPool));

        debug_name = "MaterialDescriptorPool: " + material_debug_name;
        SET_DNAME_HERE(pContext->device, materialDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, debug_name.c_str());
    }

    // Push Constants and Pipeline Layout
    VkPushConstantRange merged_push_constant{};
    if (!raw_reflected_push_constants.empty())
    {
        merged_push_constant = raw_reflected_push_constants[0];
        for (size_t i = 1; i < raw_reflected_push_constants.size(); ++i)
        {
            merged_push_constant.size = std::max(merged_push_constant.size, raw_reflected_push_constants[i].size);
            merged_push_constant.stageFlags |= raw_reflected_push_constants[i].stageFlags;
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

    debug_name = "MaterialPipelineLayout: " + material_debug_name;
    SET_DNAME_HERE(pContext->device, materialPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, debug_name.c_str());
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
    return bindingMap.contains(name);
}

ShaderBinding AnvilMaterial::getBinding(const std::string& name) const
{
    const auto it = bindingMap.find(name);
    if (it == bindingMap.end())
    {
        throw std::runtime_error("AnvilMaterial binding does not exist: " + name);
    }
    return it->second;
}

bool AnvilMaterial::hasSet(uint32_t setIndex) const
{
    return setIndex < descriptorSetLayouts.size() && descriptorSetLayouts[setIndex] != VK_NULL_HANDLE;
}

void AnvilMaterial::destroyMaterial() const
{
    if (pContext)
    {
        vertexShader.destroyShaderModule();
        fragmentShader.destroyShaderModule();

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
        if (materialPipelineLayout)
        {
            vkDestroyPipelineLayout(pContext->device, materialPipelineLayout, nullptr);
        }
    }
}

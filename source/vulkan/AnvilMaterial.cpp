// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilMaterial.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>

void AnvilMaterial::reflectShader(slang::IComponentType* linkedProgram,
    VkShaderStageFlagBits stage,
    std::vector<VkDescriptorSetLayoutBinding>& layoutBindings,
    std::vector<VkPushConstantRange>& pushConstants)
{
    slang::ShaderReflection* reflection = linkedProgram->getLayout();
    const uint32_t paramCount = reflection->getParameterCount();

    for (uint32_t i = 0; i < paramCount; i++)
    {
        slang::VariableLayoutReflection* varLayout = reflection->getParameterByIndex(i);
        slang::TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();

        const char* name = varLayout->getName();
        const slang::ParameterCategory category = varLayout->getCategory();

        if (category == slang::ParameterCategory::PushConstantBuffer)
        {
            VkPushConstantRange range{};
            range.stageFlags = stage;
            range.offset = static_cast<uint32_t>(varLayout->getOffset());
            range.size = static_cast<uint32_t>(typeLayout->getSize());
            pushConstants.push_back(range);
            continue;
        }

        if (category == slang::ParameterCategory::DescriptorTableSlot ||
            category == slang::ParameterCategory::Mixed)
        {
            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding = static_cast<uint32_t>(varLayout->getBindingIndex());
            layoutBinding.descriptorCount = 1;
            layoutBinding.stageFlags = stage;

            slang::TypeReflection::Kind kind = typeLayout->getKind();
            if (kind == slang::TypeReflection::Kind::Resource)
            {
                layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            }
            else if (kind == slang::TypeReflection::Kind::ConstantBuffer)
            {
                layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
            else
            {
                continue;
            }

            layoutBindings.push_back(layoutBinding);

            ShaderBinding info{};
            info.set = varLayout->getBindingSpace();
            info.binding = layoutBinding.binding;
            info.descriptorType = layoutBinding.descriptorType;
            bindingMap[name] = info;
        }
    }
}

void AnvilMaterial::buildMaterial(AnvilVulkanContext& inContext,
    AnvilShaderCompiler& inCompiler,
    const AnvilShaders::ShaderCompileRequest& inVertReq,
    const AnvilShaders::ShaderCompileRequest& inFragReq)
{
    ptrAContext = &inContext;

    // Compile Shaders internally
    const auto _vertex_result = inCompiler.compileToSPIRV(inVertReq);
    const auto _fragment_result = inCompiler.compileToSPIRV(inFragReq);

    // Reflect Shaders
    std::vector<VkDescriptorSetLayoutBinding> _raw_bindings;
    std::vector<VkPushConstantRange> _raw_push_constants;

    if (_vertex_result.reflection)
    {
        reflectShader(_vertex_result.reflection.get(),
            VK_SHADER_STAGE_VERTEX_BIT,
            _raw_bindings,
            _raw_push_constants);
    }

    if (_fragment_result.reflection)
    {
        reflectShader(_fragment_result.reflection.get(),
            VK_SHADER_STAGE_FRAGMENT_BIT,
            _raw_bindings,
            _raw_push_constants);
    }

    // Build Vulkan Shader Modules
    if (!vertexShader.createShaderModule(ptrAContext->anvilDevice, _vertex_result))
    {
        throw std::runtime_error("Failed to create vertex shader module!");
    }
    if (!fragmentShader.createShaderModule(ptrAContext->anvilDevice, _fragment_result))
    {
        throw std::runtime_error("Failed to create fragment shader module!");
    }

    // Merge duplicate bindings
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> _merged_bindings_map;
    for (const auto& _b : _raw_bindings)
    {
        if (_merged_bindings_map.count(_b.binding))
        {
            _merged_bindings_map[_b.binding].stageFlags |= _b.stageFlags;
        }
        else
        {
            _merged_bindings_map[_b.binding] = _b;
        }
    }

    std::vector<VkDescriptorSetLayoutBinding> _final_bindings;
    std::vector<VkDescriptorPoolSize> _pool_sizes;
    for (const auto& [binding_index, binding] : _merged_bindings_map)
    {
        _final_bindings.push_back(binding);
        VkDescriptorPoolSize pool_size{};
        pool_size.type = binding.descriptorType;
        pool_size.descriptorCount = binding.descriptorCount; // 1?
        _pool_sizes.push_back(pool_size);
    }

    // Create Layouts and Allocate Sizes
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(_final_bindings.size());
    layout_info.pBindings = _final_bindings.data();
    if (vkCreateDescriptorSetLayout(ptrAContext->anvilDevice, &layout_info, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create VkDescriptorSetLayout!");
    }
}

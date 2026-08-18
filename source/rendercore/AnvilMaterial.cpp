// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilMaterial.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <ranges>

#include "DebugNames.h"
#include "VulkanResult.h"

void AnvilMaterial::reflectShader(slang::IComponentType* linkedProgram,
                                  const VkShaderStageFlagBits stage,
                                  std::vector<VkDescriptorSetLayoutBinding>& outLayoutBindings,
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
                layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            }
            else if (kind == slang::TypeReflection::Kind::ConstantBuffer)
            {
                layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
            else
            {
                continue;
            }

            outLayoutBindings.push_back(layout_binding);

            ShaderBinding shader_binding{};
            shader_binding.set = var_layout->getBindingSpace();
            shader_binding.binding = layout_binding.binding;
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

    // Reflect Shaders
    std::vector<VkDescriptorSetLayoutBinding> _raw_bindings;
    std::vector<VkPushConstantRange> _raw_push_constants;

    if (vertex_result.reflection)
    {
        reflectShader(vertex_result.reflection.get(),
            VK_SHADER_STAGE_VERTEX_BIT,
            _raw_bindings,
            _raw_push_constants);
    }

    if (fragment_result.reflection)
    {
        reflectShader(fragment_result.reflection.get(),
            VK_SHADER_STAGE_FRAGMENT_BIT,
            _raw_bindings,
            _raw_push_constants);
    }

    // Build Vulkan Shader Modules
    std::string debug_name = "MaterialVertexShader: " + inVertReq.moduleName;
    vertexShader.createShaderModule(*pContext, vertex_result, debug_name.c_str());

    debug_name = "MaterialFragmentShader: " + inFragReq.moduleName;
    fragmentShader.createShaderModule(*pContext, fragment_result, debug_name.c_str());

    // Merge duplicate bindings
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> _merged_bindings_map;
    for (const auto& _b : _raw_bindings)
    {
        if (_merged_bindings_map.contains(_b.binding))
        {
            _merged_bindings_map[_b.binding].stageFlags |= _b.stageFlags;
        }
        else
        {
            _merged_bindings_map[_b.binding] = _b;
        }
    }

    std::vector<VkDescriptorSetLayoutBinding> final_bindings;
    std::vector<VkDescriptorPoolSize> pool_sizes;

    // for (auto it=_merged_bindings_map.begin() ; it!=_merged_bindings_map.end() ; ++it)
    for (const auto& binding : _merged_bindings_map | std::views::values)
    {
        final_bindings.push_back(binding);

        VkDescriptorPoolSize pool_size{};
        pool_size.type = binding.descriptorType;
        pool_size.descriptorCount = binding.descriptorCount * 1000; // 1000 material instances

        pool_sizes.push_back(pool_size);
    }

    // Create Layouts and Allocate Sizes
    VkDescriptorSetLayoutCreateInfo desc_layout_info{};
    desc_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    desc_layout_info.bindingCount = static_cast<uint32_t>(final_bindings.size());
    desc_layout_info.pBindings = final_bindings.data();
    CHECK(vkCreateDescriptorSetLayout(pContext->device, &desc_layout_info, nullptr, &materialDescriptorSetLayout));

    debug_name = "MaterialDescriptorSetLayout: " + material_debug_name;
    VulkanDebug::SetAutoName(pContext->device, materialDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, debug_name.c_str());

    if (!pool_sizes.empty())
    {
        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = 1000; //_pool_sizes.size()?
        CHECK(vkCreateDescriptorPool(pContext->device, &pool_info, nullptr, &materialDescriptorPool));

        debug_name = "MaterialDescriptorPool: " + material_debug_name;
        VulkanDebug::SetAutoName(pContext->device, materialDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, debug_name.c_str());
    }

    // Push Constants and Pipeline Layout
    VkPushConstantRange merged_push_constant{};
    if (!_raw_push_constants.empty())
    {
        merged_push_constant = _raw_push_constants[0];
        for (size_t i = 1; i < _raw_push_constants.size(); ++i)
        {
            merged_push_constant.size = std::max(merged_push_constant.size, _raw_push_constants[i].size);
            merged_push_constant.stageFlags |= _raw_push_constants[i].stageFlags;
        }

        pushConstantStages = merged_push_constant.stageFlags;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (materialDescriptorSetLayout != VK_NULL_HANDLE)
    {
        pipeline_layout_info.setLayoutCount = 1; //static_cast<uint32_t>(_final_bindings.size())?
        pipeline_layout_info.pSetLayouts = &materialDescriptorSetLayout;
    }
    if (merged_push_constant.size > 0)
    {
        pipeline_layout_info.pushConstantRangeCount = 1; //?
        pipeline_layout_info.pPushConstantRanges = &merged_push_constant;
    }
    CHECK(vkCreatePipelineLayout(pContext->device, &pipeline_layout_info, nullptr, &materialPipelineLayout));

    debug_name = "MaterialPipelineLayout: " + material_debug_name;
    VulkanDebug::SetAutoName(pContext->device, materialPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, debug_name.c_str());
}

MaterialInstance AnvilMaterial::createInstance() const
{
    MaterialInstance instance;
    instance.pContext = pContext;
    instance.pParentMaterial = this;

    if (materialDescriptorPool != VK_NULL_HANDLE && materialDescriptorSetLayout != VK_NULL_HANDLE)
    {
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = materialDescriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &materialDescriptorSetLayout;

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
        if (materialDescriptorSetLayout)
        {
            vkDestroyDescriptorSetLayout(pContext->device, materialDescriptorSetLayout, nullptr);
        }
        if (materialPipelineLayout)
        {
            vkDestroyPipelineLayout(pContext->device, materialPipelineLayout, nullptr);
        }
    }
}

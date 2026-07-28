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

    if (!reflection)
    {
        std::cerr << "Failed to get Slang reflection layout!" << std::endl;
        return;
    }

    uint32_t paramCount = reflection->getParameterCount();

    std::cout << "ParamCount received in AnvilMaterial: " << paramCount << std::endl;

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

            // SLANG FIX: A ConstantBuffer<T> is a wrapper. We need the size of 'T' (the element).
            slang::TypeLayoutReflection* elementType = typeLayout->getElementTypeLayout();
            if (elementType != nullptr) {
                range.size = static_cast<uint32_t>(elementType->getSize());
            } else {
                range.size = static_cast<uint32_t>(typeLayout->getSize());
            }

            pushConstants.push_back(range);

            std::cout << "Reflected Push Constant: " << name << " Size: " << range.size << "\n";
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
        if (_merged_bindings_map.contains(_b.binding))
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
    VkDescriptorSetLayoutCreateInfo desc_layout_info{};
    desc_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    desc_layout_info.bindingCount = static_cast<uint32_t>(_final_bindings.size());
    desc_layout_info.pBindings = _final_bindings.data();
    if (vkCreateDescriptorSetLayout(ptrAContext->anvilDevice, &desc_layout_info, nullptr, &materialDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Material VkDescriptorSetLayout!");
    }

    if (!_pool_sizes.empty())
    {
        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<uint32_t>(_pool_sizes.size());
        pool_info.pPoolSizes = _pool_sizes.data();
        pool_info.maxSets = 1; //_pool_sizes.size()?
        if (vkCreateDescriptorPool(ptrAContext->anvilDevice, &pool_info, nullptr, &materialDescriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Material VkDescriptorPool!");
        }

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = materialDescriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &materialDescriptorSetLayout;
        if (vkAllocateDescriptorSets(ptrAContext->anvilDevice, &alloc_info, &materialDescriptorSet) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate Material VkDescriptorSet!");
        }
    }

    // Push Constants and Pipeline Layout
    VkPushConstantRange _merged_push_constant{};
    if (!_raw_push_constants.empty())
    {
        _merged_push_constant = _raw_push_constants[0];
        for (size_t i = 1; i < _raw_push_constants.size(); ++i)
        {
            _merged_push_constant.size = std::max(_merged_push_constant.size, _raw_push_constants[i].size);
            _merged_push_constant.stageFlags |= _raw_push_constants[i].stageFlags;
        }

        pushConstantStages = _merged_push_constant.stageFlags;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (materialDescriptorSetLayout != VK_NULL_HANDLE)
    {
        pipeline_layout_info.setLayoutCount = 1; //static_cast<uint32_t>(_final_bindings.size())?
        pipeline_layout_info.pSetLayouts = &materialDescriptorSetLayout;
    }
    if (_merged_push_constant.size > 0)
    {
        pipeline_layout_info.pushConstantRangeCount = 1; //?
        pipeline_layout_info.pPushConstantRanges = &_merged_push_constant;
    }
    if (vkCreatePipelineLayout(ptrAContext->anvilDevice, &pipeline_layout_info, nullptr, &materialPipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Material VkPipelineLayout!");
    }
}

void AnvilMaterial::bindTexture(const std::string& name, const AnvilTexture& inTexture)
{
    if (!bindingMap.contains(name))
    {
        return;
    }

    const ShaderBinding _shader_binding = bindingMap[name];

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = inTexture.imageView;
    image_info.sampler = inTexture.sampler;
    pendingImageInfos.push_back(image_info);

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstBinding = _shader_binding.binding;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = _shader_binding.descriptorType;
    descriptor_write.descriptorCount = 1; //?
    pendingWrites.push_back(descriptor_write);
}

void AnvilMaterial::bindUniformBuffer(const std::string& name, const AnvilBuffer& inBuffer)
{
    (void)name;
    (void)inBuffer;
    throw std::runtime_error("AnvilMaterial::bindUniformBuffer is not implemented!");
}

void AnvilMaterial::updateDescriptorSets()
{
    if (pendingWrites.empty())
    {
        return;
    }

    size_t image_index = 0;
    size_t buffer_index = 0;

    for (auto& write : pendingWrites)
    {
        write.dstSet = materialDescriptorSet;
        if (write.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
            write.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        {
            write.pImageInfo = &pendingImageInfos[image_index];
            image_index++;
        }
        else if (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            write.pBufferInfo = &pendingBufferInfos[buffer_index];
            buffer_index++;
        }
    }

    vkUpdateDescriptorSets(ptrAContext->anvilDevice,
        static_cast<uint32_t>(pendingWrites.size()),
        pendingWrites.data(),
        0, nullptr);

    pendingWrites.clear();
    pendingImageInfos.clear();
    pendingBufferInfos.clear();
}

void AnvilMaterial::destroyMaterial() const
{
    if (ptrAContext)
    {
        vertexShader.destroyShaderModule();
        fragmentShader.destroyShaderModule();

        if (materialDescriptorPool)
        {
            vkDestroyDescriptorPool(ptrAContext->anvilDevice, materialDescriptorPool, nullptr);
        }
        if (materialDescriptorSetLayout)
        {
            vkDestroyDescriptorSetLayout(ptrAContext->anvilDevice, materialDescriptorSetLayout, nullptr);
        }
        if (materialPipelineLayout)
        {
            vkDestroyPipelineLayout(ptrAContext->anvilDevice, materialPipelineLayout, nullptr);
        }
    }
}

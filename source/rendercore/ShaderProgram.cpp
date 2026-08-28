// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "ShaderProgram.h"

#include <iostream>

void ShaderProgram::reflectStage(slang::IComponentType* linkedProgram, VkShaderStageFlagBits stage)
{
    slang::ShaderReflection* reflection = linkedProgram->getLayout();

    if (!reflection)
    {
        std::cerr << "Failed to get Slang reflection layout!" << std::endl;
        return;
    }

    const uint32_t param_count = reflection->getParameterCount();
    std::cout << "ParamCount received in AnvilMaterial: " << param_count << std::endl;

    for (uint32_t i = 0; i < param_count; i++)
    {
        slang::VariableLayoutReflection* var_layout = reflection->getParameterByIndex(i);
        slang::TypeLayoutReflection* type_layout = var_layout->getTypeLayout();

        const char* param_name = var_layout->getName();
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

            rawReflectedPushConstants.push_back(range);

            std::cout << "Reflected Push Constant: " << param_name << " Size: " << range.size << "\n";
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

            rawReflectedBindings.push_back(reflected_binding);

            ShaderBinding shader_binding{};
            shader_binding.setIndex = var_layout->getBindingSpace();
            shader_binding.bindingIndex = layout_binding.binding;
            shader_binding.descriptorType = layout_binding.descriptorType;
            bindingMap[param_name] = shader_binding;
        }
    }
}

bool ShaderProgram::buildProgram(VulkanContext& inContext, ShaderCompiler& inCompiler, const AnvilShaders::ShaderCompileRequest& inVertReq,
    const AnvilShaders::ShaderCompileRequest& inFragReq, std::string* outErrorMessage)
{
    pContext = &inContext;
    name = inVertReq.moduleName;

    const auto vertex_result = inCompiler.compileToSPIRV(inVertReq);
    const auto fragment_result = inCompiler.compileToSPIRV(inFragReq);

    if (!vertex_result.isValid() || !fragment_result.isValid())
    {
        const std::string err = "=== Vertex Shader Errors ===\n" + vertex_result.errorMessage +
                              "\n=== Fragment Shader Errors ===\n" + fragment_result.errorMessage;

        // Only write the error message if the caller provided a string pointer
        if (outErrorMessage)
        {
            *outErrorMessage = err;
        }
        else
        {
            throw std::runtime_error(err);
        }
        return false;
    }

    // Clear previous state
    rawReflectedBindings.clear();
    rawReflectedPushConstants.clear();
    bindingMap.clear();

    if (vertex_result.reflection)
    {
        reflectStage(vertex_result.reflection.get(), VK_SHADER_STAGE_VERTEX_BIT);
    }
    if (fragment_result.reflection)
    {
        reflectStage(fragment_result.reflection.get(), VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    vertexShader.createShaderModule(inContext, vertex_result DNAME((inVertReq.moduleName + inVertReq.entryPoint).c_str()));
    fragmentShader.createShaderModule(inContext, fragment_result DNAME((inFragReq.moduleName + inFragReq.entryPoint).c_str()));

    return true;
}

void ShaderProgram::destroyProgram()
{
    if (pContext)
    {
        vertexShader.destroyShaderModule();
        fragmentShader.destroyShaderModule();
    }

    rawReflectedBindings.clear();
    rawReflectedPushConstants.clear();
    bindingMap.clear();
    name.clear();
}

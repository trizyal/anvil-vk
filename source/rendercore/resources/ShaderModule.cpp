// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "ShaderModule.h"

#include <utility>

#include "VulkanResult.h"

ShaderModule::ShaderModule(ShaderModule&& other) noexcept
{
    *this = std::move(other);
}

ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept
{
    if (this != &other)
    {
        shaderModule = other.shaderModule;
        device = other.device;

        other.shaderModule = VK_NULL_HANDLE;
        other.device = VK_NULL_HANDLE;
    }
    return *this;
}

void ShaderModule::createShaderModule(const VulkanContext& inContext, const AnvilShaders::ShaderCompileResult& inSPIRV
        ANVIL_DEBUG_DEFN)
{
    device = inContext.anvilDevice;
    if (!inSPIRV.isValid())
    {
        throw std::runtime_error("Cannot create shader module from invalid SPIR-V bytecode.");
    }

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // codeSize expects bytes, but our vector holds 32-bit (4 byte) integers
    create_info.codeSize = inSPIRV.spirv.size() * sizeof(uint32_t);
    create_info.pCode = inSPIRV.spirv.data();

    CHECK(vkCreateShaderModule(device, &create_info, nullptr, &shaderModule));

    ANVIL_DEBUG_NAME(device, shaderModule, VK_OBJECT_TYPE_SHADER_MODULE);
}

void ShaderModule::destroyShaderModule() const
{
    if (shaderModule != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }
}

VkShaderModule ShaderModule::get() const
{
    return shaderModule;
}

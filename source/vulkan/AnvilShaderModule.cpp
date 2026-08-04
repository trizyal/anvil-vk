// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilShaderModule.h"

#include <iostream>
#include <utility>

AnvilShaderModule::AnvilShaderModule(AnvilShaderModule&& other) noexcept
{
    *this = std::move(other);
}

AnvilShaderModule& AnvilShaderModule::operator=(AnvilShaderModule&& other) noexcept
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

bool AnvilShaderModule::createShaderModule(const AnvilVulkanContext& inContext, const AnvilShaders::ShaderCompileResult& inSPIRV
                                           ANVIL_DEBUG_DEFN)
{
    device = inContext.anvilDevice;
    if (!inSPIRV.isValid())
    {
        std::cerr << "Cannot create shader module from invalid SPIR-V bytecode." << std::endl;
        return false;
    }

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // codeSize expects bytes, but our vector holds 32-bit (4 byte) integers
    create_info.codeSize = inSPIRV.spirv.size() * sizeof(uint32_t);
    create_info.pCode = inSPIRV.spirv.data();

    if (vkCreateShaderModule(device, &create_info, nullptr, &shaderModule) != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan shader module!\n";
        return false;
    }

    ANVIL_DEBUG_NAME(device, shaderModule, VK_OBJECT_TYPE_SHADER_MODULE);
    return true;
}

void AnvilShaderModule::destroyShaderModule() const
{
    if (shaderModule != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }
}

VkShaderModule AnvilShaderModule::get() const
{
    return shaderModule;
}

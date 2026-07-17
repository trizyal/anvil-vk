// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilShaderModule.h"

#include <iostream>


bool AnvilShaderModule::create(const VkDevice inDevice, const AnvilShaders::ShaderByteCode& inSPIRV)
{
    anvilDevice = inDevice;
    if (!inSPIRV.isValid())
    {
        std::cerr << "Cannot create shader module from invalid SPIR-V bytecode." << std::endl;
        return false;
    }

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // codeSize expects bytes, but our vector holds 32-bit (4 byte) integers
    createInfo.codeSize = inSPIRV.spirv.size() * sizeof(uint32_t);
    createInfo.pCode = inSPIRV.spirv.data();

    if (vkCreateShaderModule(anvilDevice, &createInfo, nullptr, &anvilShaderModule) != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan shader module!\n";
        return false;
    }

    return true;
}

void AnvilShaderModule::destroy() const
{
    if (anvilShaderModule != VK_NULL_HANDLE && anvilDevice != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(anvilDevice, anvilShaderModule, nullptr);
    }
}

VkShaderModule AnvilShaderModule::get() const
{
    return anvilShaderModule;
}

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilShaders.h"

#include <fstream>
#include <iostream>

#ifdef SHADERC
#include <shaderc/shaderc.hpp>
#endif //SHADERC

namespace AnvilShaders
{
#ifdef SHADERC
    shaderc_shader_kind ConvertToShadercKind(const ShaderType inShaderType)
    {
        switch (inShaderType)
        {
        case ST_Vertex:
            return shaderc_vertex_shader;

        case ST_Fragment:
            return shaderc_fragment_shader;

        case ST_Compute:
            return shaderc_compute_shader;

        default:
            return shaderc_glsl_infer_from_source;
        }
    }
#else
    SlangStage ConvertToSlangStage(ShaderType inShaderType)
    {
        switch (inShaderType)
        {
        case ST_Vertex:
            return SLANG_STAGE_VERTEX;

        case ST_Fragment:
            return SLANG_STAGE_FRAGMENT;

        case ST_Compute:
            return SLANG_STAGE_COMPUTE;

        default:
            return SLANG_STAGE_NONE;
        }
    }
#endif //SHADERC

    void DumpSPIRVToFile(std::vector<uint32_t> inSPIRV, std::string& filename)
    {
        // Open in binary mode!
        std::ofstream file(filename, std::ios::out | std::ios::binary);
        if (file.is_open()) {
            // Cast the uint32_t array to a char array so file.write can process it bytes-wise
            file.write(reinterpret_cast<const char*>(inSPIRV.data()), inSPIRV.size() * sizeof(uint32_t));
            file.close();
            std::cout << "Saved SPIR-V dump to: " << filename << "\n";
        } else {
            std::cerr << "Failed to open file for SPIR-V dump: " << filename << "\n";
        }
    }

} //AnvilShaders

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

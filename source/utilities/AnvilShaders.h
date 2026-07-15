// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SHADERS_H
#define ANVIL_VK_SHADERS_H

#include <string>
#include <vector>
#include <filesystem>

#include <volk.h>

#ifdef SHADERC
#include <shaderc/shaderc.hpp>
#endif

#ifdef SLANG
#include <slang.h>
#endif

namespace AnvilShaders
{
    enum ShaderType : uint8_t
    {
        ST_Vertex       = 0,
        ST_Fragment     = 1,
        ST_Compute      = 2,

        ST_MAX          = 3
    };

    struct ShaderCompileRequest
    {
        std::string moduleName;
        std::string entryPoint;
        ShaderType shaderType;
    };

    struct ShaderByteCode
    {
        std::vector<uint32_t> spirv;

        [[nodiscard]] bool isValid() const
        {
            return !spirv.empty();
        }
    };

    void DumpSPIRVToFile(std::vector<uint32_t> inSPIRV, std::string& filename);

#ifdef SHADERC
    shaderc_shader_kind ConvertToShadercKind(ShaderType inShaderType);
#else
    SlangStage ConvertToSlangStage(ShaderType inShaderType);
#endif //SHADERC
} //AnvilShaders

class AnvilShaderModule
{
private:
    VkDevice anvilDevice = VK_NULL_HANDLE;
    VkShaderModule anvilShaderModule = VK_NULL_HANDLE;

public:
    bool create(VkDevice inDevice, const AnvilShaders::ShaderByteCode& inSPIRV);
    void destroy() const;

    [[nodiscard]] VkShaderModule get() const;
};

#endif //ANVIL_VK_SHADERS_H

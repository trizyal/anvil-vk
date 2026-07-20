// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SHADERS_H
#define ANVIL_VK_SHADERS_H

#include <string>
#include <vector>
#include <filesystem>

#include <slang.h>

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

    SlangStage ConvertToSlangStage(ShaderType inShaderType);
} //AnvilShaders

#endif //ANVIL_VK_SHADERS_H

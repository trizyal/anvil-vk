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
        std::string filePath;
        std::string entryPoint;
        ShaderType shaderType;
    };

    struct ShaderByteCode
    {
        std::vector<uint32_t> spirv;
    };

    ShaderByteCode GetEmptyShaderByteCode();

#ifdef SHADERC
    shaderc_shader_kind ConvertToShadercKind(ShaderType inShaderType);
#endif //SHADERC
} //AnvilShaders

class AnvilShaderModule
{
private:
    VkDevice anvilDevice = VK_NULL_HANDLE;
    VkShaderModule anvilShaderModule = VK_NULL_HANDLE;

public:
    void create(VkDevice inDevice, AnvilShaders::ShaderByteCode inSPIRV);
    void destroy();

    [[nodiscard]] VkShaderModule get() const;
};

#endif //ANVIL_VK_SHADERS_H

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SHADERCOMPILER_H
#define ANVIL_VK_SHADERCOMPILER_H

#include <string>
#include <vector>

#include "AnvilShaders.h"

namespace AnvilShaderCompiler
{
    AnvilShaders::ShaderByteCode CompileToSPIRV(const AnvilShaders::ShaderCompileRequest& request);

#ifdef SHADERC
    extern bool DumpDebugSPIRV;

    std::vector<uint32_t> CompileGLSLToSPIRV(
        const std::string& glslSource,
        const std::string& shaderName,
        AnvilShaders::ShaderType inShaderType
    );
#endif //SHADERC
} // namespace AnvilShaderCompiler

#endif //ANVIL_VK_SHADERCOMPILER_H

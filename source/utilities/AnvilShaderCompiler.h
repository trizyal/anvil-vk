#ifndef ANVIL_VK_SHADERCOMPILER_H
#define ANVIL_VK_SHADERCOMPILER_H

#include <string>
#include <vector>

#include <volk.h>

#include "AnvilShaders.h"

namespace AnvilShaderCompiler
{
    extern bool DumpDebugSPIRV;

    std::vector<uint32_t> CompileGLSLToSPIRV(
        const std::string& glslSource,
        const std::string& shaderName,
        AnvilShaders::ShaderType inShaderType
    );

    // Extract bindings and push constants
    AnvilShaders::ShaderReflectionData CreateShaderReflectionData(
        const std::vector<uint32_t>& spirvCode
    );

    AnvilShaders::CompiledShader LoadShader(
        const VkDevice& device,
        const std::string& filePath,
        AnvilShaders::ShaderType shaderType
    );

    void DumpSPIRV(
        const std::string& originalPath,
        const std::vector<uint32_t>& spirvCode
    );
} // namespace AnvilShaderCompiler

#endif //ANVIL_VK_SHADERCOMPILER_H

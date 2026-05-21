#ifndef ANVIL_VK_SHADERCOMPILER_H
#define ANVIL_VK_SHADERCOMPILER_H

#include <string>
#include <vector>

#include <volk.h>

#include "AnvilShaderTypes.h"

namespace AnvilShaderCompiler
{
    extern bool DumpDebugSPIRV;

    std::vector<uint32_t> CompileGLSLToSPIRV(
        const std::string& glslSource,
        const std::string& shaderName,
        AnvilShaderTypes::ShaderType shaderType
    );

    VkShaderModule CreateShaderModule(
        const VkDevice& device,
        const std::vector<uint32_t>& spirvCode
    );
} // namespace AnvilShaderCompiler

#endif //ANVIL_VK_SHADERCOMPILER_H

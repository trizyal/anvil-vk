#ifndef ANVIL_VK_SHADERCOMPILER_H
#define ANVIL_VK_SHADERCOMPILER_H

#include <string>
#include <vector>

#include <volk.h>

namespace AnvilShaderCompiler
{
    enum ShaderType : uint8_t
    {
        ST_Vertex      = 0,
        ST_Fragment    = 1,
        ST_Compute    = 2,

        ST_MAX          = 3
    };


    std::vector<uint32_t> CompileGLSLToSPIRV(
        const std::string& glslSource,
        const std::string& shaderName,
        ShaderType shaderType
    );

    VkShaderModule CreateShaderModule(
        const VkDevice& device,
        const std::vector<uint32_t>& spirvCode
    );
}

#endif //ANVIL_VK_SHADERCOMPILER_H

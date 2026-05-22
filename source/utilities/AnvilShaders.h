#ifndef ANVIL_VK_SHADERS_H
#define ANVIL_VK_SHADERS_H

#include <string>
#include <vector>
#include <filesystem>

#include <volk.h>
#include <shaderc/shaderc.hpp>

namespace AnvilShaders
{
    enum ShaderType : uint8_t
    {
        ST_Vertex       = 0,
        ST_Fragment     = 1,
        ST_Compute      = 2,

        ST_MAX          = 3
    };

    // Extracted from SPIRV-Reflect
    struct ShaderReflectionData {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        std::vector<VkPushConstantRange> pushConstants;
    };

    // Loaded Shader
    struct CompiledShader {
        VkShaderModule module = VK_NULL_HANDLE;
        ShaderReflectionData reflection;
    };

    // Tracker for reloading
    struct ShaderSourceTracker {
        std::string filePath;
        std::filesystem::file_time_type lastCompileTime;
        ShaderType shaderType;
    };

    shaderc_shader_kind ConvertToShadercKind(ShaderType inShaderType);
}

#endif //ANVIL_VK_SHADERS_H

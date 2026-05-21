#include "AnvilShaderCompiler.h"

#include <iostream>
#include <shaderc/shaderc.hpp>

#include "AnvilFileIO.h"

using namespace AnvilShaders;

namespace AnvilShaderCompiler
{
    std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& glslSource, const std::string& shaderName, const ShaderType inShaderType)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions shaderCompileOptions;

        // TODO: optimization level should be configurable
        shaderCompileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

        shaderc::SpvCompilationResult spirvResult = compiler.CompileGlslToSpv(
                glslSource,
                ConvertToShadercKind(inShaderType),
                shaderName.c_str(),
                shaderCompileOptions
        );

        if (spirvResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            std::cerr << "Compilation failed: " << spirvResult.GetErrorMessage() << std::endl;
            throw std::runtime_error("Failed to compile shader: " + shaderName);
        }

        return {spirvResult.cbegin(), spirvResult.cend()};
    }

    ShaderReflectionData CreateShaderReflectionData(const std::vector<uint32_t>& spirvCode)
    {
        ShaderReflectionData reflectionData;

        return reflectionData;
    }

    CompiledShader LoadShader(const VkDevice& device, const std::string& filePath, ShaderType shaderType)
    {
        auto source = AnvilFileIO::readFile(filePath);
        auto spirv = CompileGLSLToSPIRV(source.data(), filePath, shaderType);

        CompiledShader temp;
        return temp;
    }

    void DumpSPIRV(const std::string& originalPath, const std::vector<uint32_t>& spirvCode)
    {

    }
}

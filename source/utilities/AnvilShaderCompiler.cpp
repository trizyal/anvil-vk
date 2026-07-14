// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only


#include "AnvilShaderCompiler.h"

#include <array>
#include <iostream>

#ifdef SHADERC
#include <shaderc/shaderc.hpp>
#endif //SHADERC

#ifdef SLANG
#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>
#endif //SLANG

#include "AnvilFileIO.h"

#include "AnvilShaders.h"
using namespace AnvilShaders;

#ifdef SLANG
void printSlangErrors(slang::IBlob* slangBlob)
{
    if (slangBlob && slangBlob->getBufferSize() > 0)
    {
        std::cerr << "Slang Compiler Error/Warning:\n"
            << static_cast<const char*>(slangBlob->getBufferPointer()) << std::endl;
    }
}
#endif //SLANG

namespace AnvilShaderCompiler
{
    ShaderByteCode GetEmptyShaderByteCode()
    {
        ShaderByteCode empty;
        std::vector<uint32_t> emptyVec;
        emptyVec.clear();
        empty.spirv = emptyVec;
        return empty;
    }

    ShaderByteCode CompileToSPIRV(const ShaderCompileRequest& request)
    {
        // Slang Global Session, Ideally only done once
        Slang::ComPtr<slang::IGlobalSession> globalSession;
        if (SLANG_FAILED(slang::createGlobalSession(globalSession.writeRef())))
        {
            std::cerr << "Failed to create Slang Global Session" << std::endl;
        }

        // Setup target and session (Vulkan SPIR-V)
        slang::TargetDesc targetDesc = {};
        targetDesc.format = SLANG_SPIRV;
        // Target Vulkan 1.3 / SPIR-V 1.5.
        targetDesc.profile = globalSession->findProfile("spirv_1_5");

        slang::SessionDesc sessionDesc = {};
        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        // Compiler options
        std::array<slang::CompilerOptionEntry, 1> options =
        {
        {
                slang::CompilerOptionName::EmitSpirvDirectly,
                slang::CompilerOptionValue{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
            }
        };
        sessionDesc.compilerOptionEntries = options.data();
        sessionDesc.compilerOptionEntryCount = options.size();

        Slang::ComPtr<slang::ISession> session;
        globalSession->createSession(sessionDesc, session.writeRef());

        // Load the Module (e.g.,
    }

#ifdef SHADERC
    std::vector<uint32_t> CompileGLSLToSPIRV(const std::string& glslSource, const std::string& shaderName,
                                             const ShaderType inShaderType)
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
#endif //SHADERC
}

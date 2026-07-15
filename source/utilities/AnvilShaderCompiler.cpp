// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only


#include "AnvilShaderCompiler.h"

#include <array>
#include <iostream>

#ifdef SLANG
#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>
#endif //SLANG

#include <fstream>

#include "AnvilFileIO.h"

#include "AnvilShaders.h"
using namespace AnvilShaders;

ShaderByteCode GetEmptyShaderByteCode()
{
    ShaderByteCode empty;
    std::vector<uint32_t> emptyVec;
    emptyVec.clear();
    empty.spirv = emptyVec;
    return empty;
}

#ifdef SLANG
void DiagnoseIfNeeded(slang::IBlob* slangBlob)
{
    if (slangBlob && slangBlob->getBufferSize() > 0)
    {
        std::cerr << "Slang Compiler Error/Warning:\n"
            << static_cast<const char*>(slangBlob->getBufferPointer()) << std::endl;
    }
}
#endif //SLANG

bool AnvilShaderCompiler::init()
{
    if (SLANG_FAILED(slang::createGlobalSession(globalSession.writeRef())))
    {
        std::cerr << "Failed to create Slang Global Session." << std::endl;
        return false;
    }
    return true;
}

void AnvilShaderCompiler::destroy()
{
    // Explicitly release the COM pointer to free Slang resources
    globalSession.setNull();
}

void AnvilShaderCompiler::setOptimizationLevel(const OptimizationLevel inLevel)
{
    optimizationLevel = inLevel;
}

void AnvilShaderCompiler::addSearchPath(const std::string& inPath)
{
    searchPaths.push_back(inPath);
}

void AnvilShaderCompiler::setSpirvDump(const bool inEnable, const std::string& inDumpDirectory)
{
    bDumpSpirv = inEnable;
    dumpDirectory = inDumpDirectory;
}

int32_t AnvilShaderCompiler::getSlangOptimizationLevel(const OptimizationLevel inLevel)
{
    switch (inLevel)
    {
    case OptimizationLevel::None:
        return SLANG_OPTIMIZATION_LEVEL_NONE;
    case OptimizationLevel::Default:
        return SLANG_OPTIMIZATION_LEVEL_DEFAULT;
    case OptimizationLevel::High:
        return SLANG_OPTIMIZATION_LEVEL_HIGH;
    default:
        return SLANG_OPTIMIZATION_LEVEL_DEFAULT;
    }
}


ShaderByteCode AnvilShaderCompiler::compileToSPIRV(const ShaderCompileRequest& request) const
{
    ShaderByteCode retByteCode = GetEmptyShaderByteCode();
    if (!globalSession)
    {
        std::cerr << "Slang Global Session is not initialized!\n";
        return retByteCode;
    }

    // Setup target (Vulkan 1.3 / SPIR-V 1.5)
    slang::TargetDesc targetDesc = {};
    targetDesc.format = SLANG_SPIRV;
    targetDesc.profile = globalSession->findProfile("spirv_1_5");

    // Configure Session
    slang::SessionDesc sessionDesc = {};
    sessionDesc.targets = &targetDesc;
    sessionDesc.targetCount = 1;

    // Apply Search Paths
    std::vector<const char*> cSearchPaths;
    for (const auto& path : searchPaths)
    {
        cSearchPaths.push_back(path.c_str());
    }

    if (cSearchPaths.empty())
    {
        const char* defaultPath[] = {"shaders"};
        sessionDesc.searchPaths = defaultPath;
        sessionDesc.searchPathCount = 1;
    }
    else
    {
        sessionDesc.searchPaths = cSearchPaths.data();
        sessionDesc.searchPathCount = static_cast<uint32_t>(cSearchPaths.size());
    }

    // Compiler options
#if 0
    std::array<slang::CompilerOptionEntry, 1> options =
    {
        {
            slang::CompilerOptionName::EmitSpirvDirectly,
            slang::CompilerOptionValue{slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr}
        }
    };
#endif

    // Apply Optimization level
    const slang::CompilerOptionEntry options = {
         slang::CompilerOptionName::Optimization,
          { slang::CompilerOptionValueKind::Int, getSlangOptimizationLevel(optimizationLevel), 0, 0, 0 }
    };
    sessionDesc.compilerOptionEntries = &options;
    sessionDesc.compilerOptionEntryCount = 1;

    // Create the Session
    Slang::ComPtr<slang::ISession> session;
    globalSession->createSession(sessionDesc, session.writeRef());

    // Load the Shader Modules
    Slang::ComPtr<slang::IBlob> diagnostics;
    slang::IModule* slangModule = session->loadModule(request.moduleName.c_str(), diagnostics.writeRef());
    DiagnoseIfNeeded(diagnostics);

    if (!slangModule)
    {
        std::cerr << "AnvilShaderCompiler: Failed to load Slang module: " << request.moduleName.c_str() << std::endl;
        return retByteCode;
    }

    // Find the Entry Points
    // eg. [shader("vertex")] vertexMain in Slang
    Slang::ComPtr<slang::IEntryPoint> entryPoint;
    slangModule->findEntryPointByName(request.entryPoint.c_str(), entryPoint.writeRef());

    if (!entryPoint)
    {
        std::cerr << "AnvilShaderCompiler: Failed to find entry point " << request.entryPoint.c_str() << std::endl;
        return retByteCode;
    }

    // Composite and Link
    slang::IComponentType* componentTypes[] = {slangModule, entryPoint};
    Slang::ComPtr<slang::IComponentType> compositeComponent;
    session->createCompositeComponentType(
        componentTypes,
        2,
        compositeComponent.writeRef(),
        diagnostics.writeRef()
    );
    DiagnoseIfNeeded(diagnostics);

    Slang::ComPtr<slang::IComponentType> linkedProgram;
    compositeComponent->link(linkedProgram.writeRef(), diagnostics.writeRef());
    DiagnoseIfNeeded(diagnostics);

    // Extract SPIR-V
    Slang::ComPtr<slang::IBlob> spirvBlob;
    linkedProgram->getTargetCode(0, spirvBlob.writeRef(), diagnostics.writeRef());
    DiagnoseIfNeeded(diagnostics);
    if (spirvBlob)
    {
        const uint32_t* spirvCode = static_cast<const uint32_t*>(spirvBlob->getBufferPointer());
        const size_t spirvWordCount = spirvBlob->getBufferSize() / sizeof(uint32_t);
        retByteCode.spirv.assign(spirvCode, spirvCode + spirvWordCount);
    }

    // Dump SPIR-V if requested
    if (bDumpSpirv && retByteCode.isValid()) {
        std::string fileName = request.entryPoint + ".spv";
        std::string fullPath = dumpDirectory.empty() ? fileName : (dumpDirectory + "/" + fileName);

        std::ofstream file(fullPath, std::ios::out | std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(retByteCode.spirv.data()), retByteCode.spirv.size() * sizeof(uint32_t));
            file.close();
        }
        else
        {
            std::cerr << "AnvilShaderCompiler: Failed to open dump file: " << fullPath << "\n";
        }
    }

    return retByteCode;
}

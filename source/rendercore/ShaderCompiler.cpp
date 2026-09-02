// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only


#include "ShaderCompiler.h"

#include <array>
#include <iostream>
#include <fstream>

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>

#include "AnvilShaders.h"
using namespace AnvilShaders;

namespace
{
    ShaderCompileResult GetEmptyShaderByteCode()
    {
        ShaderCompileResult empty;
        std::vector<uint32_t> empty_vec;
        empty_vec.clear();
        empty.spirv = empty_vec;
        return empty;
    }

    void DiagnoseIfNeeded(slang::IBlob* slangBlob, std::string& outErrorMessage)
    {
        if (slangBlob && slangBlob->getBufferSize() > 0)
        {
            const char* diagnostic_text = static_cast<const char*>(slangBlob->getBufferPointer());
            std::cerr << "Slang Compiler Error/Warning:" << std::endl;
            std::cerr << diagnostic_text << std::endl;
            outErrorMessage += diagnostic_text;
            outErrorMessage += '\n';
        }
    }
} //Anonymous

bool ShaderCompiler::initializeShaderCompiler()
{
    if (SLANG_FAILED(slang::createGlobalSession(globalSession.writeRef())))
    {
        std::cerr << "Failed to create Slang Global Session." << std::endl;
        return false;
    }
    searchPaths.emplace_back(ANVIL_SHADER_DIR);
    return true;
}

void ShaderCompiler::shutdownShaderCompiler()
{
    // Explicitly release the COM pointer to free Slang resources
    session.setNull();
    globalSession.setNull();
}

void ShaderCompiler::setOptimizationLevel(const OptimizationLevel inLevel)
{
    optimizationLevel = inLevel;
}

void ShaderCompiler::addSearchPath(const std::string& inPath)
{
    searchPaths.push_back(inPath);
}

void ShaderCompiler::setSpirvDump(const bool inEnable, const std::string& inDumpDirectory)
{
    bDumpSpirv = inEnable;
    dumpDirectory = inDumpDirectory;
}

void ShaderCompiler::resetSession()
{
    session.setNull();
}

int32_t ShaderCompiler::getSlangOptimizationLevel(const OptimizationLevel inLevel)
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


ShaderCompileResult ShaderCompiler::compileToSPIRV(const ShaderCompileRequest& request)
{
    ShaderCompileResult shader_result = GetEmptyShaderByteCode();
    if (!globalSession)
    {
        throw std::runtime_error("Slang Global Session is not initialized.");
    }

    if (!session)
    {
        // Setup target (Vulkan 1.3 / SPIR-V 1.5)
        slang::TargetDesc target_desc = {};
        target_desc.format = SLANG_SPIRV;
        target_desc.profile = globalSession->findProfile("spirv_1_5");

        // Configure Session
        slang::SessionDesc session_desc = {};
        session_desc.targets = &target_desc;
        session_desc.targetCount = 1;

        // Vulkan uses column major
        session_desc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

        // Apply Search Paths
        std::vector<const char*> search_paths;
        for (const auto& path : searchPaths)
        {
            search_paths.push_back(path.c_str());
        }

        // Needs to be initialized outside the .empty() block
        const char* defaultPath[] = {ANVIL_SHADER_DIR};

        if (search_paths.empty())
        {
            session_desc.searchPaths = defaultPath;
            session_desc.searchPathCount = 1;
        }
        else
        {
            session_desc.searchPaths = search_paths.data();
            session_desc.searchPathCount = static_cast<uint32_t>(search_paths.size());
        }

        // Apply Optimization level
        const slang::CompilerOptionEntry options = {
            .name = slang::CompilerOptionName::Optimization,
            .value = {
                .kind = slang::CompilerOptionValueKind::Int,
                .intValue0 = getSlangOptimizationLevel(optimizationLevel),
                .intValue1 = 0,
                .stringValue0 = nullptr, .stringValue1 = nullptr
            }
        };
        session_desc.compilerOptionEntries = &options;
        session_desc.compilerOptionEntryCount = 1;

        // Create the Session
        globalSession->createSession(session_desc, session.writeRef());
    }

    // Load the Slang Shader Modules
    Slang::ComPtr<slang::IBlob> diagnostics_blob;
    slang::IModule* slang_module = session->loadModule(request.moduleName.c_str(), diagnostics_blob.writeRef());
    DiagnoseIfNeeded(diagnostics_blob.get(), shader_result.errorMessage);

    if (!slang_module)
    {
        std::string err = "AnvilShaderCompiler: Failed to load Slang module: " + request.moduleName;
        std::cerr << err << std::endl;
        if (shader_result.errorMessage.empty())
        {
            shader_result.errorMessage = err;
        }
        return shader_result;
    }

    // Find the Entry Points
    // eg. [shader("vertex")] vertexMain in Slang
    Slang::ComPtr<slang::IEntryPoint> entry_point;
    slang_module->findEntryPointByName(request.entryPoint.c_str(), entry_point.writeRef());

    if (!entry_point)
    {
        std::string err = "AnvilShaderCompiler: Failed to find entry point " + request.entryPoint;
        std::cerr << err << std::endl;
        shader_result.errorMessage = err;
        return shader_result;
    }

    // Composite and Link
    slang::IComponentType* component_types[] = {slang_module, entry_point};
    Slang::ComPtr<slang::IComponentType> composite_component;
    session->createCompositeComponentType(
        component_types,
        2,
        composite_component.writeRef(),
        diagnostics_blob.writeRef()
    );
    DiagnoseIfNeeded(diagnostics_blob, shader_result.errorMessage);
    if (!composite_component)
    {
        return shader_result;
    }

    Slang::ComPtr<slang::IComponentType> linked_program;
    composite_component->link(linked_program.writeRef(), diagnostics_blob.writeRef());
    DiagnoseIfNeeded(diagnostics_blob, shader_result.errorMessage);
    if (!linked_program)
    {
        return shader_result;
    }

    // Extract SPIR-V
    Slang::ComPtr<slang::IBlob> spirv_blob;
    linked_program->getTargetCode(0, spirv_blob.writeRef(), diagnostics_blob.writeRef());
    DiagnoseIfNeeded(diagnostics_blob, shader_result.errorMessage);

    if (spirv_blob)
    {
        const auto* spirv_code = static_cast<const uint32_t*>(spirv_blob->getBufferPointer());
        const size_t spirv_word_count = spirv_blob->getBufferSize() / sizeof(uint32_t);
        shader_result.spirv.assign(spirv_code, spirv_code + spirv_word_count);
    }

    shader_result.reflection = linked_program;
    return shader_result;
}

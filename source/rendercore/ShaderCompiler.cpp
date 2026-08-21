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

    void DiagnoseIfNeeded(slang::IBlob* slangBlob)
    {
        if (slangBlob && slangBlob->getBufferSize() > 0)
        {
            std::cerr << "Slang Compiler Error/Warning:" << std::endl;
            std::cerr << static_cast<const char*>(slangBlob->getBufferPointer()) << std::endl;
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

    // Load the Shader Modules
    Slang::ComPtr<slang::IBlob> diagnostics_blob;
    slang::IModule* slang_module = session->loadModule(request.moduleName.c_str(), diagnostics_blob.writeRef());
    DiagnoseIfNeeded(diagnostics_blob);

    if (!slang_module)
    {
        std::cerr << "AnvilShaderCompiler: Failed to load Slang module: " << request.moduleName.c_str() << std::endl;
        return shader_result;
    }

    // Find the Entry Points
    // eg. [shader("vertex")] vertexMain in Slang
    Slang::ComPtr<slang::IEntryPoint> entry_point;
    slang_module->findEntryPointByName(request.entryPoint.c_str(), entry_point.writeRef());

    if (!entry_point)
    {
        std::cerr << "AnvilShaderCompiler: Failed to find entry point " << request.entryPoint.c_str() << std::endl;
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
    DiagnoseIfNeeded(diagnostics_blob);

    Slang::ComPtr<slang::IComponentType> linked_program;
    composite_component->link(linked_program.writeRef(), diagnostics_blob.writeRef());
    DiagnoseIfNeeded(diagnostics_blob);

    // Extract SPIR-V
    Slang::ComPtr<slang::IBlob> spirv_blob;
    linked_program->getTargetCode(0, spirv_blob.writeRef(), diagnostics_blob.writeRef());
    DiagnoseIfNeeded(diagnostics_blob);

    if (spirv_blob)
    {
        const uint32_t* spirv_code = static_cast<const uint32_t*>(spirv_blob->getBufferPointer());
        const size_t spirv_word_count = spirv_blob->getBufferSize() / sizeof(uint32_t);
        shader_result.spirv.assign(spirv_code, spirv_code + spirv_word_count);
    }

    shader_result.reflection = linked_program;

    auto param_count = linked_program->getLayout()->getParameterCount();

    std::cout << "Parameter count in ShaderCompiler: " << param_count << std::endl;

    // TODO: This dump should ideally be in readable code
    // Dump SPIR-V if requested
    if (bDumpSpirv && shader_result.isValid()) {
        std::string file_name = request.entryPoint + ".spv";
        std::string full_path = dumpDirectory.empty() ? file_name : (dumpDirectory + "/" + file_name);

        std::ofstream file(full_path, std::ios::out | std::ios::binary);
        if (file.is_open()) {
            file.write(
                reinterpret_cast<const char*>(shader_result.spirv.data()),
                static_cast<std::streamsize>(shader_result.spirv.size() * sizeof(uint32_t))
            );
            file.close();
        }
        else
        {
            std::cerr << "AnvilShaderCompiler: Failed to open dump file: " << full_path << "\n";
        }
    }

    return shader_result;
}

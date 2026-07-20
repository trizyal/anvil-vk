// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SHADERCOMPILER_H
#define ANVIL_VK_SHADERCOMPILER_H

#include <slang.h>
#include <slang-com-ptr.h>

#include "AnvilShaders.h"

class AnvilShaderCompiler
{
public:
    enum class OptimizationLevel : uint8_t
    {
        None    = 0, // Best for debugging
        Default = 1, // Standard compilation
        High    = 2, // Aggressive optimizations
    };

    // Initialise the Global Session
    bool init();

    // Clean up compiler resources
    void destroy();

    // Configuration Setters
    void setOptimizationLevel(OptimizationLevel inLevel);
    void addSearchPath(const std::string& inPath);
    void setSpirvDump(bool inEnable, const std::string& inDumpDirectory);

private:
    Slang::ComPtr<slang::IGlobalSession> globalSession;
    static int32_t getSlangOptimizationLevel(OptimizationLevel inLevel);

    // Configuration State
    OptimizationLevel optimizationLevel = OptimizationLevel::Default;
    std::vector<std::string> searchPaths;
    bool bDumpSpirv = false;
    std::string dumpDirectory;

public:
    // Request compiler shaders from AnvilShaderCompiler
    AnvilShaders::ShaderByteCode compileToSPIRV(const AnvilShaders::ShaderCompileRequest& request) const;
};

#endif //ANVIL_VK_SHADERCOMPILER_H

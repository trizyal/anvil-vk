// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SHADERCOMPILER_H
#define ANVIL_VK_SHADERCOMPILER_H

/**
 * @file AnvilShaderCompiler.h
 * @brief Runtime Slang-to-SPIR-V shader compilation utility wrapping the Slang SDK.
 */

#include <slang.h>
#include <slang-com-ptr.h>

#include "AnvilShaders.h"

/**
 * @brief Runtime compiler wrapper around the Slang compilation API.
 *
 * Manages Slang global sessions and compilation contexts to translate Slang shader code
 * into Vulkan-compatible SPIR-V bytecode at runtime. Supports configurable include paths,
 * optimization tiers, and optional SPIR-V binary dumping for offline inspection.
 *
 * @note This class is non-copyable and non-movable.
 *
 * @todo SPIRV dumps should be readable.
 */
class AnvilShaderCompiler
{
public:
    AnvilShaderCompiler() = default;
    ~AnvilShaderCompiler() = default;

    AnvilShaderCompiler(const AnvilShaderCompiler&) = delete;
    AnvilShaderCompiler& operator=(const AnvilShaderCompiler&) = delete;
    AnvilShaderCompiler(AnvilShaderCompiler&&) = delete;
    AnvilShaderCompiler& operator=(AnvilShaderCompiler&&) = delete;

    /**
     * @brief Optimization levels applied during Slang-to-SPIR-V compilation.
     */
    enum class OptimizationLevel : uint8_t
    {
        None    = 0, /**< No optimizations applied; preserves debug symbols and flow for debugging. */
        Default = 1, /**< Standard optimization balance suitable for general development. */
        High    = 2  /**< Aggressive performance optimizations for release builds. */
    };

private:
    Slang::ComPtr<slang::IGlobalSession> globalSession;
    Slang::ComPtr<slang::ISession> session;

    // Configuration State
    OptimizationLevel optimizationLevel = OptimizationLevel::Default;
    std::vector<std::string> searchPaths;
    bool bDumpSpirv = false;
    std::string dumpDirectory;

public:
    /**
     * @brief Bootstraps the Slang global session and creates an active compilation session.
     * @return `true` if the Slang API initialized successfully, `false` otherwise.
     */
    bool initializeShaderCompiler();

    /**
     * @brief Releases active Slang sessions and frees underlying COM pointers.
     */
    void shutdownShaderCompiler();

    /**
     * @brief Sets the compiler optimization level for subsequent compile requests.
     * @param inLevel The optimization tier to apply (None, Default, or High).
     */
    void setOptimizationLevel(OptimizationLevel inLevel);

    /**
     * @brief Registers an additional directory to search for shaders.
     * @param inPath Absolute or relative folder path containing shader files.
     */
    void addSearchPath(const std::string& inPath);

    /**
     * @brief Configures whether compiled SPIR-V binaries should be written to disk for inspection.
     * @param inEnable `true` to enable disk dumping of SPIR-V artifacts, `false` to disable.
     * @param inDumpDirectory Output directory where `.spv` binaries will be written if enabled.
     */
    void setSpirvDump(bool inEnable, const std::string& inDumpDirectory);

    /**
     *@brief Compiles a Slang shader source file or entry point into Vulkan SPIR-V bytecode.
     *
     * Uses the currently configured search paths and optimization levels to process the
     * shader request, returning either the compiled SPIR-V binary payload or diagnostic errors.
     *
     * @param request Struct containing source file path, entry point name, and target stage.
     * @return Result structure containing the compiled SPIR-V buffer and reflection data.
     *
     * @bug Should throw a 
     */
    AnvilShaders::ShaderCompileResult compileToSPIRV(const AnvilShaders::ShaderCompileRequest& request);

private:
    static int32_t getSlangOptimizationLevel(OptimizationLevel inLevel);
};

#endif //ANVIL_VK_SHADERCOMPILER_H

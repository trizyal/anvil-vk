// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SHADERS_H
#define ANVIL_VK_SHADERS_H

/**
 * @file AnvilShaders.h
 * @brief Data types and helper functions for Slang shader compilation requests and SPIR-V results.
 */

#include <string>
#include <vector>
#include <filesystem>

#include <slang.h>
#include <slang-com-ptr.h>

/**
 * @brief Functions and structures to support shader compilation
 */
namespace AnvilShaders
{
    /**
     * @brief Supported pipeline shader execution stages.
     */
    enum ShaderType : uint8_t
    {
        ST_Vertex                       = 0, /**< Vertex shader stage. */
        ST_Fragment                     = 1, /**< Fragment (pixel) shader stage. */
        ST_Compute [[maybe_unused]]     = 2, /**< Compute shader stage. */

        ST_MAX                          = 3  /**< Total count of supported shader types. */
    };

    /**
     * @brief Parameters identifying the source module and entry point to be compiled.
     */
    struct ShaderCompileRequest
    {
        std::string moduleName; /**< File name or module path of the Slang shader. */
        std::string entryPoint; /**< Name of the shader entry point function (e.g., "vertexMain"). */

        [[maybe_unused]]
        ShaderType shaderType;  /**< Target pipeline execution stage for this entry point. */
    };

    /**
     * @brief Output artifact containing compiled SPIR-V bytecode and Slang reflection metadata.
     */
    struct ShaderCompileResult
    {
        std::vector<uint32_t> spirv;
        Slang::ComPtr<slang::IComponentType> reflection;
        std::string errorMessage; /**< Contains Slang warnings and error logs. */

        /**
         * @brief Checks whether compilation produced valid SPIR-V bytecode.
         * @return `false` if the SPIR-V vector is empty, otherwise `true`.
         */
        [[nodiscard]] bool isValid() const
        {
            return !spirv.empty();
        }
    };

    /**
     * @brief Writes compiled SPIR-V bytecode words to a binary file on disk.
     *
     * Useful for debugging compiler artifacts or generating offline `.spv` caches.
     * @param inSPIRV Vector of 32-bit SPIR-V binary words to write.
     * @param filename Destination file path where the binary artifact will be saved.
     */
    void DumpSPIRVToFile(const std::vector<uint32_t>& inSPIRV, const std::string& filename);

    /**
     * @brief Translates an Anvil ShaderType into the equivalent Slang SDK stage enumeration.
     * @param inShaderType The internal Anvil shader stage to convert.
     * @return Corresponding SlangStage value required by the Slang API.
     */
    SlangStage ConvertToSlangStage(ShaderType inShaderType);
} //AnvilShaders

#endif //ANVIL_VK_SHADERS_H

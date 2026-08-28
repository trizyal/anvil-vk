// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SHADERPROGRAM_H
#define ANVIL_VK_SHADERPROGRAM_H

/**
 * @file ShaderProgram.h
 * @brief Encapsulates shader modules and their Slang reflection data.
 */

#include <vector>
#include <unordered_map>
#include <string>

#include <volk.h>
#include <slang.h>

#include "ShaderModule.h"
#include "ShaderCompiler.h"

class VulkanContext;

/**
 * @brief Cached Vulkan descriptor binding metadata extracted via Slang shader reflection.
 */
struct ShaderBinding
{
    /** Descriptor set index (e.g., set = 0) */
    uint32_t setIndex;

    /** Binding slot index within the descriptor set. */
    uint32_t bindingIndex;

    /** Vulkan resource type (e.g., VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER). */
    VkDescriptorType descriptorType;
};

struct ReflectedBinding
{
    uint32_t setIndex;
    VkDescriptorSetLayoutBinding bindingData;
};

/**
 * @brief Owns compiled shader stages and their reflected pipeline layouts.
 */
class ShaderProgram
{
public:
    ShaderProgram() = default;
    ~ShaderProgram() = default;

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&&) noexcept = default;
    ShaderProgram& operator=(ShaderProgram&&) noexcept = default;

    std::string name;

    /** Owned vertex shader module and associated SPIR-V bytecode. */
    ShaderModule vertexShader;

    /** Owned fragment shader module and associated SPIR-V bytecode. */
    ShaderModule fragmentShader;

    /**
     * @brief A flat list storing every individual descriptor binding discovered in the shaders.
     *
     * If a resource (like a Scene UBO) is used in BOTH the Vertex and
     * Fragment shaders, it will be added to this list twice.
     * */
    std::vector<ReflectedBinding> rawReflectedBindings;

    /**
     * @brief A flat list of push constant ranges extracted from the shaders.
     *
     * Like bindings, if multiple shader stages use push constants, we will get multiple entries here that need to be combined later.
     */
    std::vector<VkPushConstantRange> rawReflectedPushConstants;


    std::unordered_map<std::string, ShaderBinding> bindingMap;

private:
    VulkanContext* pContext = nullptr;

public:
    /**
     * @brief Compiles shaders and extracts reflection data.
     * @param inContext Active Vulkan context.
     * @param inCompiler Configured Slang compiler.
     * @param inVertReq Vertex shader request.
     * @param inFragReq Fragment shader request.
     * @param outErrorMessage String to append errors to.
     *
     * @return `true` if compilation succeeded, `false` otherwise.
     */
    bool buildProgram(VulkanContext& inContext,
                      ShaderCompiler& inCompiler,
                      const AnvilShaders::ShaderCompileRequest& inVertReq,
                      const AnvilShaders::ShaderCompileRequest& inFragReq,
                      std::string* outErrorMessage = nullptr);

    void destroyProgram();

private:
    /**
     * @brief Inspects Slang reflection metadata to populate descriptor layouts and push constant ranges.
     *
     * @param[in] linkedProgram Slang component type containing layout reflection data.
     * @param[in] stage Target Vulkan shader stage bitmask for these bindings.
     */
    void reflectStage(slang::IComponentType* linkedProgram, VkShaderStageFlagBits stage);
};

#endif //ANVIL_VK_SHADERPROGRAM_H

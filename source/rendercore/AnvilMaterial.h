// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_MATERIAL_H
#define ANVIL_VK_MATERIAL_H

/**
 * @file AnvilMaterial.h
 * @brief Factory class managing Slang shader compilation, reflection, and Vulkan layouts.
 */

#include <string>

#include <volk.h>

#include "MaterialInstance.h"
#include "ShaderModule.h"
#include "ShaderProgram.h"
#include "VulkanContext.h"

/**
 * @brief Encapsulates shaders and acts as a factory for Material Instances.
 *
 * Owns the vertex and fragment shader modules. Uses Slang reflection metadata during build time
 * to automatically generate Vulkan descriptor set layouts, pipeline layouts, and descriptor pools.
 * Creates and dispenses `AnvilMaterialInstance` objects for rendering.
 *
 * @note This class in non-copyable. Moving is allowed.
 *
 * @warning Only stores one pair of vertex and fragment shaders, may need re-architecting.
 */
class AnvilMaterial
{
public:
    AnvilMaterial() = default;
    ~AnvilMaterial() = default;

    AnvilMaterial(const AnvilMaterial&) = delete;
    AnvilMaterial& operator=(const AnvilMaterial&) = delete;

    AnvilMaterial(AnvilMaterial&&) noexcept = default;
    AnvilMaterial& operator=(AnvilMaterial&&) noexcept = default;

    /** Reflected layouts for the material's descriptor sets (Index 0 = Set 0, etc). */
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

    /** Pool allocated specifically for this material's descriptor sets. */
    VkDescriptorPool materialDescriptorPool = VK_NULL_HANDLE;

    /** Layout describing descriptor sets and push constants for this material. */
    VkPipelineLayout materialPipelineLayout = VK_NULL_HANDLE;

    /** Bitmask of pipeline stages utilizing push constants in this material. */
    VkShaderStageFlags pushConstantStages = 0;

    const ShaderProgram* pActiveProgram = nullptr;

private:
    VulkanContext* pContext = nullptr;

public:
    /**
     * @brief Builds layout using an already compiled ShaderProgram.
     *
     * @param inContext
     * @param inProgram
     */
    void buildMaterialFromProgram(VulkanContext& inContext, const ShaderProgram& inProgram);

    /**
     * @brief Destroys all Vulkan layouts, descriptor pools, and shader modules owned by this material.
     */
    void destroyMaterial();

    /**
     * @brief Allocates a new instance for a specific Descriptor Set Index.
     *
     * @param setIndex The index this new set exists in.
     * @return A ready-to-use MaterialInstance.
     */
    [[nodiscard]]
    MaterialInstance allocateSet(uint32_t setIndex) const;

    /**
     * @brief Returns true if this material reflected a binding with the given shader variable name.
     */
    [[nodiscard]]
    bool hasBinding(const std::string& name) const;

    /**
     * @brief Retrieves reflected binding metadata by shader variable name.
     *
     * @throws std::runtime_error If the binding does not exist.
     */
    [[nodiscard]]
    ShaderBinding getBinding(const std::string& name) const;

    /**
     * @brief Checks if a specific descriptor set index has been successfully allocated.
     * @param setIndex The descriptor set index to verify (e.g., 0, 1, 2).
     * @return True if the set layout exists and is not null, false otherwise.
     */
    [[nodiscard]]
    bool hasSet(uint32_t setIndex) const;

    /**
     * @brief Retrieves the compiled vertex shader module.
     * @return The VkShaderModule handle, or VK_NULL_HANDLE if no program is active.
     */
    [[nodiscard]]
    VkShaderModule getVertexShader() const
    {
        return pActiveProgram ? pActiveProgram->vertexShader.get() : VK_NULL_HANDLE;
    }

    /**
     * @brief Retrieves the compiled fragment shader module.
     * @return The VkShaderModule handle, or VK_NULL_HANDLE if no program is active.
     */
    [[nodiscard]]
    VkShaderModule getFragmentShader() const
    {
        return  pActiveProgram ? pActiveProgram->fragmentShader.get() : VK_NULL_HANDLE;
    }
};

#endif //ANVIL_VK_MATERIAL_H

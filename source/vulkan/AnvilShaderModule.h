// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SHADERMODULE_H
#define ANVIL_VK_SHADERMODULE_H

/**
 * @file AnvilShaderModule.h
 * @brief Move-only wrapper around Vulkan shader modules compiled from Slang/SPIR-V bytecode.
 */

#include <volk.h>

#include "AnvilShaders.h"
#include "AnvilVulkanContext.h"
#include "AnvilVulkanDebug.h"

/**
 * @brief Manages the lifecycle of a Vulkan shader module (`VkShaderModule`).
 *
 * Caches the parent logical device upon creation to allow a parameter free destruction.
 * Implements move-only semantics to prevent accidental double-deletion
 * of GPU shader handles when passed across scopes or stored in containers.
 *
 * @note Copying this class is disallowed. Moving is allowed.
 */
class AnvilShaderModule
{
public:
    AnvilShaderModule() = default;
    ~AnvilShaderModule() = default;

    // Disallow copying to prevent double destruction/creation
    AnvilShaderModule(const AnvilShaderModule&) = delete;
    AnvilShaderModule& operator=(const AnvilShaderModule&) = delete;

    // Allow moving
    AnvilShaderModule(AnvilShaderModule&&) noexcept;
    AnvilShaderModule& operator=(AnvilShaderModule&&) noexcept;

    VkShaderModule shaderModule = VK_NULL_HANDLE;

private:
    VkDevice device = VK_NULL_HANDLE;

public:
    bool createShaderModule(const AnvilVulkanContext& inContext, const AnvilShaders::ShaderCompileResult& inSPIRV ANVIL_DEBUG_DECL());
    void destroyShaderModule() const;

    [[nodiscard]] VkShaderModule get() const;
};

#endif //ANVIL_VK_SHADERMODULE_H

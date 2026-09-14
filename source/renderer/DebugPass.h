// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_DEBUGPASS_H
#define ANVIL_VK_DEBUGPASS_H

/**
 * @file DebugPass.h
 * @brief Engine-level subsystem managing universal debug rendering pipelines.
 */

#include <volk.h>
#include "AnvilMaterial.h"
#include "PipelineBuilder.h"
#include "ShaderProgram.h"

class VulkanContext;
class ShaderCompiler;
class GBuffer;

/**
 * @brief Render view modes supported by the debug pass.
 */
enum class DebugMode : uint32_t
{
    None = 0,
    BaseColor,
    GeometryNormal,
    RawNormalMap,
    WorldNormal,
    Metallic,
    Roughness,
    Depth,
    OverdrawComplexity,
    OvershadingComplexity,
    Count
};

/**
 * @brief Manages engine-injected debug pipelines for both Forward and Deferred rendering.
 */
class DebugPass
{
private:
    VulkanContext* pContext = nullptr;

public:
    ShaderProgram program_Deferred;
    AnvilMaterial material_Deferred;
    AnvilPipeline pipeline_Deferred;
    MaterialInstance set_Deferred;

    ShaderProgram program_Forward;
    AnvilMaterial material_Forward;
    AnvilPipeline pipeline_Forward_Opaque;
    AnvilPipeline pipeline_Forward_Overdraw;
    AnvilPipeline pipeline_Forward_Overshading;

    /**
     * @brief Compiles and initializes all engine debug pipelines.
     */
    void initializeDebugPass(VulkanContext& inContext, ShaderCompiler& inCompiler, VkFormat swapchainFormat,
                             VkFormat depthFormat);

    /**
     * @brief Destroys all debug pipelines and layouts.
     */
    void cleanupDebugPass();

    /**
     * @brief Returns true if the requested debug mode requires Forward rendering geometry intercepts.
     */
    static bool isForwardMode(uint32_t mode);

    /**
     * @brief Returns true if the requested debug mode requires Deferred fullscreen reading.
     */
    static bool isDeferredMode(uint32_t mode);

    /**
     * @brief Retrieves the correct Forward pipeline based on the requested debug mode.
     */
    [[nodiscard]] AnvilPipeline getForwardPipeline(uint32_t mode) const;

    /**
     * @brief Retrieves the unified Forward debug pipeline layout.
     */
    VkPipelineLayout getForwardLayout() const;

    /**
     * @brief Binds the deferred debug shader and reads the G-Buffer to output the debug view.
     */
    void drawDeferredResolve(VkCommandBuffer cmd, GBuffer& gBuffer, uint32_t mode);
};


#endif //ANVIL_VK_DEBUGPASS_H

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
#include "DebugModes.h"
#include "PipelineBuilder.h"
#include "ShaderProgram.h"
#include "glm/vec4.hpp"

class VulkanContext;
class ShaderCompiler;
class GBuffer;

/**
 * @brief Push constant block mapped to the deferred debug shader.
 */
struct DebugDeferredPushConstants
{
    glm::vec4 cameraPosition;
    DebugMode debugMode;
};

/**
 * @brief Manages engine-injected debug pipelines for both Forward and Deferred rendering.
 */
class DebugPass
{
private:
    VulkanContext* pContext = nullptr;

public:
    /** Compiled shader program for deferred debug resolution. */
    ShaderProgram program_Deferred;

    /** Material layout for the deferred debug pipeline. */
    AnvilMaterial material_Deferred;

    /** Graphics pipeline for rendering a fullscreen deferred quad. */
    AnvilPipeline pipeline_Deferred;

    /** Descriptor set binding G-Buffer textures for deferred resolution. */
    MaterialInstance set_Deferred;

    /** Compiled shader program for forward debug rendering. */
    ShaderProgram program_Forward;

    /** Material layout for the forward debug pipeline. */
    AnvilMaterial material_Forward;

    /** Pipeline used for solid debug overlays (e.g., Normals, Albedo). */
    AnvilPipeline pipeline_Forward_Opaque;

    /** Pipeline used for additive overdraw visualization. */
    AnvilPipeline pipeline_Forward_Overdraw;

    /** Pipeline used for additive overshading visualization. */
    AnvilPipeline pipeline_Forward_Overshading;

    /** Pipeline used for wireframe rendering. */
    AnvilPipeline pipeline_Forward_Wireframe;

    /** Tracks if the G-Buffer has been recreated, requiring descriptor set updates. */
    VkImageView cachedGBufferView = VK_NULL_HANDLE;

    /**
     * @brief Compiles and initializes all engine debug pipelines.
     *
     * @param inContext Reference to the active Vulkan context.
     * @param inCompiler Reference to the shader compiler used to build the debug shaders.
     * @param swapchainFormat The pixel format of the color attachment or swapchain being rendered into.
     * @param depthFormat The pixel format of the depth attachment.
     * @param outError Optional pointer to a string that will be populated with compiler output if compilation fails.
     * @return True if all shaders compiled and pipelines initialized successfully, false otherwise.
     */
    bool initializeDebugPass(VulkanContext& inContext, ShaderCompiler& inCompiler, VkFormat swapchainFormat,
                             VkFormat depthFormat, std::string* outError = nullptr);

    /**
     * @brief Destroys all debug pipelines and layouts.
     */
    void cleanupDebugPass();

    /**
     * @brief Returns true if the requested debug mode requires Forward rendering geometry intercepts.
     *
     * @param mode The integer representation of the active DebugMode.
     * @return True if the mode requires forward rendering (e.g., Overdraw, Wireframe), false otherwise.
     */
    static bool isForwardMode(uint32_t mode);

    /**
     * @brief Returns true if the requested debug mode requires Deferred fullscreen reading.
     *
     * @param mode The integer representation of the active DebugMode.
     * @return True if the mode requires reading from the G-Buffer (e.g., BaseColor, WorldNormal), false otherwise.
     */
    static bool isDeferredMode(uint32_t mode);

    /**
     * @brief Retrieves the correct Forward pipeline based on the requested debug mode.
     *
     * @param mode The integer representation of the active DebugMode.
     * @return The AnvilPipeline configured for the requested debug visualization.
     */
    [[nodiscard]] AnvilPipeline getForwardPipeline(uint32_t mode) const;

    /**
     * @brief Retrieves the unified Forward debug pipeline layout.
     *
     * @return The Vulkan pipeline layout shared across all forward debug pipelines.
     */
    [[nodiscard]] VkPipelineLayout getForwardLayout() const;

    /**
     * @brief Binds the deferred debug shader and reads the G-Buffer to output the debug view.
     *
     * @param cmd Active Vulkan command buffer to record the fullscreen draw into.
     * @param gBuffer The populated G-Buffer to sample attachments from.
     * @param debugMode The specific deferred debug view to render.
     * @param camPos World-space position of the camera, used for reconstructing depth visuals.
     */
    void drawDeferredResolve(VkCommandBuffer cmd, GBuffer& gBuffer, DebugMode debugMode, const glm::vec4& camPos);
};


#endif //ANVIL_VK_DEBUGPASS_H

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_UIRENDERER_H
#define ANVIL_VK_UIRENDERER_H

/**
 * @file AnvilUIRenderer.h
 * @brief Manages Dear ImGui integration, overlay rendering, and debug UI widgets for Vulkan.
 */

#include <volk.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "../rendercore/context/AnvilVulkanDebug.h"

class AnvilVulkanContext;
class AnvilSwapchain;

/**
 * @brief Subsystem responsible for initializing, recording, and rendering user interface overlay frames.
 *
 * Integrates Dear ImGui with the engine's Vulkan rendering backend and GLFW window.
 * Owns the dedicated ImGui descriptor pool and caches the parent Vulkan context.
 *
 * @note This class is non-copyable and non-movable. May need to change that.
 */
class AnvilUIRenderer
{
public:
    /**
     * @brief Constructs an uninitialized UI renderer container.
     */
    AnvilUIRenderer() = default;

    /**
     * @brief Destroys the UI renderer, shutting down ImGui backends and destroying the descriptor pool.
     */
    ~AnvilUIRenderer();

    AnvilUIRenderer(const AnvilUIRenderer&) = delete;
    AnvilUIRenderer& operator=(const AnvilUIRenderer&) = delete;

    AnvilUIRenderer(AnvilUIRenderer&&) = delete;
    AnvilUIRenderer& operator=(AnvilUIRenderer&&) = delete;

    /** Target color attachment pixel format for UI rendering. */
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;

    /** Target depth attachment pixel format for UI rendering. */
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

private:
    AnvilVulkanContext* ptrAContext = nullptr;

    /** Dedicated descriptor pool allocated for Dear ImGui internal resources. */
    VkDescriptorPool imguiPool = VK_NULL_HANDLE;

public:
    /**
     * @brief Initializes the Dear ImGui GLFW and Vulkan backends, creating required fonts and descriptor pools.
     * @param inContext Pointer to the root Vulkan context providing device and queue handles.
     * @param inWindow Pointer to the active GLFW window to attach OS event callbacks to.
     * @param inSwapchain Pointer to the active swapchain to query color and depth attachment formats.
     * @return `true` if ImGui initialization succeeded, `false` otherwise.
     */
    bool initializeUIRenderer(AnvilVulkanContext* inContext, GLFWwindow* inWindow, AnvilSwapchain* inSwapchain);

    /**
     * @brief Starts a new ImGui frame for both the GLFW and Vulkan backends.
     * @note Must be called inside an active rendering block.
     */
    static void BeginUIFrame();

    /**
     * @brief Records generated ImGui draw data into the provided command buffer.
     * @note Should be called inside an active rendering block after main scene geometry.
     * @param inCmdBuffer Active Vulkan command buffer to record UI draw commands into.
     */
    static void RecordUICommands(VkCommandBuffer inCmdBuffer);

    /**
     * @brief Finalizes ImGui UI construction for the current frame.
     *
     * Assembles the internal draw lists ready for submission via RecordUICommands().
     */
    static void EndUIFrame();

    /**
     * @brief Renders a debug 3D orientation axis overlay in a corner of the viewport.
     * @param viewMatrix Current active camera view matrix used to orient the widget's axes.
     */
    static void DrawDebugAxis(const glm::mat4& viewMatrix);

private:
    /**
     * @brief Internal helper to allocate the dedicated descriptor pool required by ImGui.
     *
     * Allocates descriptor pool sizes for samplers, combined image samplers, and uniforms.
     * @see ImGuiPoolSizes.
     *
     * @param inDevice Logical Vulkan device handle used to create the pool.
     * @param aDebugName Optional debug name for Vulkan object.
     * @param aDbgSrcLoc Automatic.
     */
    void createDescriptorPool(VkDevice inDevice ANVIL_DEBUG_DECL());
};

#endif //ANVIL_VK_UIRENDERER_H

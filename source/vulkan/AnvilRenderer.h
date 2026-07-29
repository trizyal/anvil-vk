// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_RENDERER_H
#define ANVIL_VK_RENDERER_H

/**
 * @file AnvilRenderer.h
 * @brief Core frame synchronization, command recording, and draw-loop orchestration.
 */

#include <functional>
#include "AnvilSwapchain.h"

/**
 * @brief Per-frame GPU resources required for flight synchronized rendering.
 */
struct AnvilFrame
{
    /**< Pool allocated specifically for this frame's command buffer. */
    VkCommandPool cmdPool = VK_NULL_HANDLE;

    /**< Primary command buffer for recording draw commands. */
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

    /**< Signaled when the swapchain image is ready to render to. */
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;

    /**< CPU waits on this for the GPU to finish rendering the frame. */
    VkFence frameDoneFence = VK_NULL_HANDLE;
};

/**
 * @brief Number of concurrent frames the CPU can submit ahead of the GPU.
 */
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

/**
 * @brief Orchestrates the Vulkan draw loop, sync primitives, and frame timing.
 *
 * Manages multi-buffered frames in flight, handles swapchain recreation on resize,
 * and provides a callback interface for recording commands into the active buffer.
 */
class AnvilRenderer
{
public:
    AnvilRenderer() = default;
    ~AnvilRenderer() = default;

    // Delete Copy Operations (Prevents double-freeing Vulkan sync objects & pools)
    AnvilRenderer(const AnvilRenderer&) = delete;
    AnvilRenderer& operator=(const AnvilRenderer&) = delete;

    // Delete Move Operations (Locks the renderer instance in place)
    AnvilRenderer(AnvilRenderer&&) = delete;
    AnvilRenderer& operator=(AnvilRenderer&&) = delete;

private:
    AnvilVulkanContext* ptrAContext = nullptr;
    AnvilSwapchain* ptrASwapchain = nullptr;

    AnvilFrame anvilFrames[FRAMES_IN_FLIGHT];
    uint32_t anvilFrameIndex = 0;

    // Rendering is finished, ready to present
    std::vector<VkSemaphore> renderFinishedSemaphores;

    bool recreateSwapchain = false;

public:
    /**
     * @brief Initializes frame resources, command pools, and synchronization primitives.
     *
     * @param inAnvilContext Pointer to the initialized Anvil Vulkan context.
     * @param inAnvilSwapchain Pointer to the active swapchain to render into.
     *
     * @throws std::runtime_error If command pools, buffers, or sync objects fail to create.
     */
    void initializeRenderer(AnvilVulkanContext* inAnvilContext, AnvilSwapchain* inAnvilSwapchain);

    /**
     * @brief Waits for the GPU to idle and destroys all per-frame Vulkan resources.
     */
    void shutdownRenderer();

    /**
     * @brief Prepare a frame for rendering, executes the draw callback, and presents.
     *
     * Handles CPU-GPU synchronization, acquiring a swapchain image, executing user-provided
     * rendering commands, and submitting the result to the presentation queue.
     * Flags the swapchain for recreation if window resizing is detected.
     *
     * @param inWindow Reference to the Anvil Window to check for the resized or minimized state from GLFW.
     * @param drawCallback A lambda or function invoked with the active command buffer and swapchain.
     *
     * @note drawCallback is triggered after BeginRendering is called and before the UI renders.
     */
    void drawFrame(AnvilWindow& inWindow, const std::function<void(VkCommandBuffer, AnvilSwapchain*)>& drawCallback);

private:
    AnvilFrame& getCurrentFrame();
    void setupCommandBuffers();
    void setupSyncStructures();
    void transitionImageLayout(VkCommandBuffer inCmd, VkImage inImage,
        VkImageLayout oldLayout, VkImageLayout newLayout);
};

#endif //ANVIL_VK_RENDERER_H

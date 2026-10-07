// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file RenderContext.h
 */

#include <volk.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "GPUProfiler.h"
#include "Index32.h"
#include "Swapchain.h"

class Window;

/**
 * @brief Per-frame GPU resources required for flight synchronized rendering.
 */
struct Frame
{
    /** Pool allocated specifically for this frame's command buffer. */
    VkCommandPool cmdPool = VK_NULL_HANDLE;

    /** Primary command buffer for recording draw commands. */
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

    /** Signaled when the swapchain image is ready to render to. */
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;

    /** CPU waits on this for the GPU to finish rendering the frame. */
    VkFence frameDoneFence = VK_NULL_HANDLE;
};

/**
 * @brief Number of concurrent frames the CPU can submit ahead of the GPU.
 */
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

/**
 * @brief Orchestrates the Vulkan draw loop, sync primitives, and frame timing.
 *
 * Manages multi-buffered frames in flight, handles swapchain recreation on resize.
 *
 * @note This class is non-copyable and non-movable. May need to change that.
 */
class RenderContext
{
public:
    RenderContext() = default;
    ~RenderContext();

    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;

    RenderContext(RenderContext&&) = delete;
    RenderContext& operator=(RenderContext&&) = delete;

private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;

    /** Array of frame-sync structures for flighted rendering. */
    Frame frames[FRAMES_IN_FLIGHT];

    /** Signaled when rendering is finished, and image ready to present. */
    std::vector<VkSemaphore> renderFinishedSemaphores;

    /** Flag triggered when a window resize requires the swapchain to be rebuilt. */
    bool recreateSwapchain = false;

public:
    /** External Tracy context for visual GPU profiling. */
    TracyVkCtx tracyVkCtx = nullptr;

    /** Handles internal GPU timestamp queries for performance tracking. */
    GPUProfiler gpuProfiler;

    /** Tracks which CPU frame is currently being processed (modulo FRAMES_IN_FLIGHT). */
    uint32_t frameIndex = 0;

    /** Tracks which swapchain image was provided by the Vulkan presentation engine. */
    uint32_t imageIndex = 0;

    /**
     * @brief Initializes the render context, setting up command buffers and sync structures.
     * @param inContext Pointer to the initialized core Vulkan context.
     * @param inSwapchain Pointer to the application's swapchain.
     */
    void initializeRenderContext(VulkanContext* inContext, Swapchain* inSwapchain);

    /**
     * @brief Prepares the next frame for rendering by acquiring a swapchain image and resetting command buffers.
     * @param inWindow Reference to the application window, used to query extents if the swapchain requires recreation.
     * @return A valid VkCommandBuffer ready for recording on success, or VK_NULL_HANDLE if the swapchain is out of date (e.g., window minimized/resized).
     */
    VkCommandBuffer beginFrame(const Window& inWindow);

    /**
     * @brief Ends command recording, transitions the swapchain image, submits the frame to the GPU, and presents it.
     */
    void endFrame();

    /**
     * @brief Retrieves the frame data structure for the current CPU frame.
     * @return A reference to the active Frame.
     */
    Frame& getCurrentFrame()
    {
        return frames[frameIndex];
    }

private:
    /**
     * @brief Allocates command pools and buffers for all frames in flight.
     */
    void setupCommandBuffers();

    /**
     * @brief Creates semaphores and fences for CPU/GPU and Queue synchronization.
     */
    void setupSyncStructures();
};

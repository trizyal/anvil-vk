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
    VulkanContext* pContext;
    Swapchain* pSwapchain;

    Frame frames[FRAMES_IN_FLIGHT];
};

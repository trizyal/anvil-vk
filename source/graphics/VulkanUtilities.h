// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <volk.h>

#include "Swapchain.h"
#include "VulkanTypes.h"

/**
 * @file VulkanUtilities.h
 * @brief Free functions for some vulkan utilities.
 */

namespace VulkanUtils
{
    /**
     * @brief Evaluates current and required layouts and inserts a VkImageMemoryBarrier if a transition is needed.
     * @param cmd Active Vulkan command buffer.
     * @param image Target Vulkan image handle.
     * @param currentLayout Reference to the tracked layout state variable (updated upon barrier emission).
     * @param newLayout Target ImageLayout required for the upcoming operation.
     * @param isDepth True if evaluating a depth aspect image transition.
     */
    void TransitionImage(VkCommandBuffer cmd, VkImage image, ImageLayout& currentLayout, ImageLayout newLayout, bool isDepth = false);

    /**
     * @brief Helper to dynamically set the viewport and scissor rect to match the swapchain.
     *
     * @param inCmd Active command buffer.
     * @param inSwapchain The swapchain to pull the extent from.
     */
    void SetViewportScissor(VkCommandBuffer inCmd, const Swapchain& inSwapchain);
}
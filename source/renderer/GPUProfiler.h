// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_GPUPROFILER_H
#define ANVIL_VK_GPUPROFILER_H

/**
 * @file GPUProfiler.h
 * @brief Manages query pools and contexts to profile the frame.
 */

#include <volk.h>
#include <cstdint>
#include <vector>

class VulkanContext;
/**
 * @brief Subsystem to profile the GPU frame render data.
 *
 * @note This class is non-copyable and non-movable.
 */
class GPUProfiler
{
public:
    GPUProfiler() = default;
    ~GPUProfiler();

    GPUProfiler(const GPUProfiler&) = delete;
    GPUProfiler& operator=(const GPUProfiler&) = delete;
    GPUProfiler(GPUProfiler&&) = delete;
    GPUProfiler& operator=(GPUProfiler&&) = delete;

private:
    VulkanContext* pContext = nullptr;

    std::vector<VkQueryPool> queryPools;
    std::vector<uint8_t> querySubmitted;
    float timestampPeriod = 1.0f;

public:
    /**
     * @brief Initializes a timestamp query pool for each frame in flight.
     *
     * @param inContext Pointer to the root Vulkan context providing device and queue handles.
     * @param inTimePeriod idk
     * @param maxFramesInFlight Max frames in flight allowed by the renderer.
     *
     * @todo Need to replace the constant in maxFramesInFlight = 2 to a const var.
     */
    void initializeGPUProfiler(VulkanContext* inContext, float inTimePeriod, uint32_t maxFramesInFlight = 2);

    /**
     * @brief Resets the query pool and writes the start timestamp.
     *
     * @param inCmdBuffer Active Vulkan command buffer.
     * @param frameIndex Current frame actively being rendered.
     */
    void beginGPUProfilerFrame(VkCommandBuffer inCmdBuffer, uint32_t frameIndex) const;

    /**
     * @brief Writes the end timestamp.
     *
     * @param inCmdBuffer Active Vulkan command buffer.
     * @param frameIndex Current frame actively being rendered.
     */
    void endGPUProfilerFrame(VkCommandBuffer inCmdBuffer, uint32_t frameIndex);

    /**
     * @brief Retrieves the time delta between beginFrame and endFrame in milliseconds.
     * @param frameIndex Current frame.
     * @return GPU time in milliseconds.
     */
    float getGPUTime(uint32_t frameIndex) const;
};


#endif //ANVIL_VK_GPUPROFILER_H

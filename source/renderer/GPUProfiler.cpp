// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "GPUProfiler.h"

#include "VulkanContext.h"
#include "VulkanResult.h"

void GPUProfiler::initializeGPUProfiler(VulkanContext* inContext, float inTimePeriod, uint32_t maxFramesInFlight)
{
    pContext = inContext;
    timestampPeriod = inTimePeriod;

    VkQueryPoolCreateInfo query_pool_info{};
    query_pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_info.queryCount = 2; // 0 = Start, 1 = End

    queryPools.resize(maxFramesInFlight);
    querySubmitted.resize(maxFramesInFlight, 0);
    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        CHECK(vkCreateQueryPool(pContext->device, &query_pool_info, nullptr, &queryPools[i]));
    }
}

GPUProfiler::~GPUProfiler()
{
    for (VkQueryPool pool : queryPools)
    {
        if (pool != VK_NULL_HANDLE)
        {
            vkDestroyQueryPool(pContext->device, pool, nullptr);
        }
    }
    queryPools.clear();
}

void GPUProfiler::beginGPUProfilerFrame(VkCommandBuffer inCmdBuffer, const uint32_t frameIndex) const
{
    // Reset the pool before using it this frame
    vkCmdResetQueryPool(inCmdBuffer, queryPools[frameIndex], 0, 2);

    // Write Top of Pipe (before any graphics commands execute)
    vkCmdWriteTimestamp(inCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPools[frameIndex], 0);
}

void GPUProfiler::endGPUProfilerFrame(VkCommandBuffer inCmdBuffer, const uint32_t frameIndex)
{
    // Write Bottom of Pipe (after all graphics commands finish)
    vkCmdWriteTimestamp(inCmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPools[frameIndex], 1);

    // Mark this query as successfully recorded so we can safely read it next time!
    querySubmitted[frameIndex] = 1;
}

float GPUProfiler::getGPUTime(uint32_t frameIndex) const
{
    if (!querySubmitted[frameIndex])
    {
        return 0.0f;
    }

    uint64_t timestamps[2] = {0, 0};

    // Attempt to read the results. If VK_NOT_READY is returned, it means the GPU
    // hasn't finished yet (e.g., first frame, or fence wait was bypassed).
    const VkResult res = vkGetQueryPoolResults(
        pContext->device,
        queryPools[frameIndex],
        0, 2,
        sizeof(timestamps),
        timestamps,
        sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT
    );

    if (res == VK_SUCCESS)
    {
        const uint64_t delta_ticks = timestamps[1] - timestamps[0];
        // Convert ticks to nanoseconds, then to milliseconds
        return (delta_ticks * timestampPeriod) / 1000000.0f;
    }

    return 0.0f; // Data not ready yet
}

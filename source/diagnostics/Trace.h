// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file Trace.h
 */

#include <volk.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#include "Color.h"

#define SCOPE_FRAME FrameMark
#define SCOPE_CPU ZoneScopedC(Colors::Pink.t24())
#define SCOPE_CPUN(name) ZoneScopedNC(name, Colors::Purple.t24())
#define SCOPE_CPUC(color) ZoneScopedC(color.t24())
#define SCOPE_CPUNC(name, color) ZoneScopedNC(name, color.t24())

/** @brief Dynamic runtime zone names. */
#define SCOPE_CPUD(name, color) \
    ZoneScopedC(color.t24()); \
    ZoneName(name, strlen(name))

struct GPUMarkerScope
{
    VkCommandBuffer cmd;

    GPUMarkerScope(VkCommandBuffer inCmd, const char* name, Color color)
        :cmd(inCmd)
    {
        if (vkCmdBeginDebugUtilsLabelEXT)
        {
            VkDebugUtilsLabelEXT label_info{};
            label_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            label_info.pLabelName = name;
            label_info.color[0] = color.r;
            label_info.color[1] = color.g;
            label_info.color[2] = color.b;
            label_info.color[3] = color.a;
            vkCmdBeginDebugUtilsLabelEXT(cmd, &label_info);
        }
    }

    ~GPUMarkerScope()
    {
        if (vkCmdEndDebugUtilsLabelEXT)
        {
            vkCmdEndDebugUtilsLabelEXT(cmd);
        }
    }
};

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

/**
 * @brief Combined GPU Zone macro: Emits a Tracy GPU timestamp and a RenderDoc Event Marker with default color.
 */
#define SCOPE_GPU(cmdCtx, name) \
    TracyVkNamedZoneC(cmdCtx.tracyCtx, CONCAT(_tracy_gpu_zone_, __LINE__), cmdCtx.cmd, name, Colors::Gray.t24(), true); \
    GPUMarkerScope CONCAT(gpu_marker_, __LINE__)(cmdCtx.cmd, name, Colors::Gray)

/**
 * @brief Combined GPU Zone macro: Emits a Tracy GPU timestamp and a RenderDoc Event Marker.
 */
#define SCOPE_GPUC(cmdCtx, name, color) \
    TracyVkNamedZoneC(cmdCtx.tracyCtx, CONCAT(_tracy_gpu_zone_, __LINE__), cmdCtx.cmd, name, color.t24(), true); \
    GPUMarkerScope CONCAT(gpu_marker_, __LINE__)(cmdCtx.cmd, name, color)

/**
 * @brief Dynamic runtime strings using Tracy Transient allocations.
 */
#define SCOPE_GPUCD(cmdCtx, name, color) \
    TracyVkZoneTransient(cmdCtx.tracyCtx, CONCAT(_tracy_gpu_zone_, __LINE__), cmdCtx.cmd, name, true); \
    GPUMarkerScope CONCAT(gpu_marker_, __LINE__)(cmdCtx.cmd, name, color)

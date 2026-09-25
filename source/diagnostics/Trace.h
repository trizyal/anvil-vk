// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_TRACE_H
#define ANVIL_VK_TRACE_H

#include <volk.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>

#define SCOPE_FRAME FrameMark
#define SCOPE_CPU ZoneScoped
#define SCOPE_CPU_NAME(name) ZoneScopedN(name)

struct GPUMarkerScope
{
    VkCommandBuffer cmd;

    GPUMarkerScope(VkCommandBuffer inCmd, const char* name)
        :cmd(inCmd)
    {
        if (vkCmdBeginDebugUtilsLabelEXT)
        {
            VkDebugUtilsLabelEXT label_info{};
            label_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            label_info.pLabelName = name;
            label_info.color[0] = 0.2f;
            label_info.color[1] = 0.6f;
            label_info.color[2] = 1.0f;
            label_info.color[3] = 1.0f;
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
 * @brief Combined GPU Zone macro: Emits a Tracy GPU timestamp and a RenderDoc Event Marker.
 */
#define SCOPE_GPU(tracyCtx, cmd, name) \
    TracyVkNamedZone(tracyCtx, CONCAT(_tracy_gpu_zone_, __LINE__), cmd, name, true); \
    GPUMarkerScope CONCAT(gpu_marker_, __LINE__)(cmd, name)

#endif //ANVIL_VK_TRACE_H

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

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

/**
 * @brief Combined GPU Zone macro: Emits a Tracy GPU timestamp todo: AND a RenderDoc Event Marker.
 */
#define SCOPE_GPU(tracyCtx, cmd, name) \
    TracyVkZone(tracyCtx, cmd, name);

#endif //ANVIL_VK_TRACE_H

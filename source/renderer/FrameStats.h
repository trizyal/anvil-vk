// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_FRAMESTATS_H
#define ANVIL_VK_FRAMESTATS_H

#include <cstdint>

struct FrameStats
{
    float fps = 0.0f;
    float frameTime = 0.0f; //ms
    float cpuTime = 0.0f; //ms
    float gpuTime = 0.0f; //ms

    uint32_t drawCalls = 0;
    uint32_t primitiveCount = 0;

    void resetFrameStats()
    {
        drawCalls = 0;
        primitiveCount = 0;
    }
};

#endif //ANVIL_VK_FRAMESTATS_H

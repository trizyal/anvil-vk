// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_RENDERDOC_H
#define ANVIL_VK_RENDERDOC_H

#include "renderdoc_app.h"


class RenderDoc
{
public:
    /** RenderDoc API */
    static RENDERDOC_API_1_1_2* rdoc_api;

    /** Loads the API pointer before Vulkan initializes. */
    static void InitializeRenderDoc();

    /** Captures the next frame and launches the RenderDoc UI. */
    static void TriggerCapture();

    static bool IsInitialized()
    {
        return rdoc_api != nullptr;
    }
};


#endif //ANVIL_VK_RENDERDOC_H

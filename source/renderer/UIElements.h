// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_UIELEMENTS_H
#define ANVIL_VK_UIELEMENTS_H

#include "FrameStats.h"

/**
 * @file UIElements.h
 * @brief Free functions for different UI elements.
 */

#include <glm/glm.hpp>
#include <imgui.h>
#include <string>

namespace UI
{
    inline const char* FontPath = ASSETS_DIR "/fonts/Open_Sans/OpenSans-Regular.ttf";
    inline ImFont* base = nullptr;
    inline ImFont* debugUI = nullptr;
    inline ImFont* debugLog = nullptr;

    void LoadFonts();

    /**
     * @brief Render frame stats.
     *
     * @param stats Frame stats calculated in the Renderer.
     * @param pOpen Whether the stat ui is being renderered.
     */
    void FrameStats(const FrameStats& stats, bool* pOpen = nullptr);

    /**
     * @brief Renders a debug 3D orientation axis overlay in a corner of the viewport.
     * @param viewMatrix Current active camera view matrix used to orient the widget's axes.
     */
    void RenderWorldAxes(const glm::mat4& viewMatrix);

    /**
     * @brief
     *
     * @param errorLog
     * @param onRetry
     * @param onAbort
     */
    void DrawShaderErrorModal(const std::string& errorLog, const std::function<void()>& onRetry, const std::function<void()>& onAbort);
}



#endif //ANVIL_VK_UIELEMENTS_H

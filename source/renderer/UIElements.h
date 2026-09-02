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

    enum class DebugMode : uint32_t
    {
        None        = 0,
        BaseColor   = 1,
        WorldNormal = 2,
        NormalMap   = 3,
        Metallic    = 4,
        Roughness   = 5
    };

    /**
     * @brief Loads default fonts for Anvil.
     */
    void LoadFonts();

    /**
     * @brief Applies the engine-wide custom Anvil Dark UI theme and metrics.
     */
    void ApplyAnvilTheme();

    /**
     * @brief Render frame stats.
     *
     * @param stats Frame stats calculated in the Renderer.
     * @param pOpen Whether the stat ui is being rendered.
     */
    void FrameStats(const FrameStats& stats, bool* pOpen = nullptr);

    /**
     * @brief Renders a debug 3D orientation axis overlay in a corner of the viewport.
     * @param viewMatrix Current active camera view matrix used to orient the widget's axes.
     */
    void RenderWorldAxes(const glm::mat4& viewMatrix);

    /**
     * @brief Renders a modal overlay displaying shader compilation errors with options to retry or abort.
     *
     * @param errorLog The formatted diagnostic message or error output from the shader compiler.
     * @param onRetry Callback function executed when the user chooses to attempt re-compiling the shaders.
     * @param onAbort Callback function executed when the user chooses to cancel the reload and keep the existing pipeline.
     */
    void DrawShaderErrorModal(const std::string& errorLog, const std::function<void()>& onRetry, const std::function<void()>& onAbort);

    /**
     * @brief Renders a global debug view toggle menu.
     *
     * @param currentMode Reference to the active debug mode state.
     * @return True if the mode was changed this frame.
     */
    bool RenderDebugMenu(uint32_t& currentMode);
}



#endif //ANVIL_VK_UIELEMENTS_H

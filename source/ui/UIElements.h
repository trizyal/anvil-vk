// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file UIElements.h
 * @brief Free functions for different UI elements.
 */

#include <string>

#include <glm/glm.hpp>

#include "SceneConfig.h"
#include "FrameStats.h"

struct UILogMessage;
class SceneManager;
struct ImFont;
class Camera;

namespace UI
{
    /** Relative path to the default TrueType font used by ImGui. */
    inline const char* FontPath = ASSETS_DIR "/fonts/Open_Sans/OpenSans-Regular.ttf";

    /** The base font handle used for standard UI text. */
    inline ImFont* base = nullptr;

    /** A smaller font variant used for debug statistics and compact overlays. */
    inline ImFont* debugUI = nullptr;

    /** Font handle designated for the developer console logs. */
    inline ImFont* debugLog = nullptr;

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
     * @brief Renders the developer console overlay window.
     *
     * Reads directly from the static Console backend to display logs and handle
     * command execution parsing via ImGui input text buffers.
     *
     * @param pState Pointer to the state integer (0 = Closed, 1 = Mini, 2 = Full).
     */
    void DrawConsoleWindow(const int* pState);

    extern bool bShowCameraDebug;

    /**
     * @brief Renders a debug window showing live camera coordinates and rotation.
     *
     * @param camera The active camera to inspect and edit.
     * @param pOpen Optional boolean pointer to toggle the window.
     */
    void DrawCameraDebug(Camera& camera, bool* pOpen = nullptr);

    void DrawMainMenuBar();
    bool DrawScenesMenu(SceneManager& sceneManager);
    void DrawLogOverlay(const std::vector<UILogMessage>& messages);
    void DrawViewsMenu();
}

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_UILOGGER_H
#define ANVIL_VK_UILOGGER_H

/**
 * @file AnvilUILogger.h
 * @brief Thread-safe on-screen UI logger and ImGui overlay for runtime diagnostics.
 */

#include <string>
#include <vector>
#include <mutex>

#include <imgui.h>

/**
 * @brief Predefined RGBA ImVec4 color constants for styling on-screen log messages.
 */
namespace AnvilColor
{
    inline constexpr ImVec4 Green  = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); /**< Default log color for general success/status messages. */
    inline constexpr ImVec4 Red    = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); /**< Error messages and fatal failures. */
    inline constexpr ImVec4 Yellow = ImVec4(1.0f, 1.0f, 0.2f, 1.0f); /**< Warnings and non-fatal alerts. */
    inline constexpr ImVec4 Blue   = ImVec4(0.3f, 0.7f, 1.0f, 1.0f); /**< Informational or verbose engine events. */
    inline constexpr ImVec4 White  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); /**< Neutral or unstyled text. */
    inline constexpr ImVec4 Gray   = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); /**< High-frequency debug spam or low-priority output. */
} //AnvilColor

/**
 * @brief Internal container representing a single timed log notification.
 */
struct UILogMessage
{
    std::string text;    /**< Message string to display on screen. */
    ImVec4 color;        /**< RGBA color for rendering the text in ImGui. */
    float timeRemaining; /**< Remaining display time in seconds before being removed. */
};

/**
 * @brief Default UI Log display time.
 */
constexpr float LOG_DISPLAY_TIME = 5.0f;

/**
 * @brief Thread-safe static logging service that renders notifications as an ImGui overlay.
 * 
 * Manages an internal message queue protected by a mutex, allowing safe log submissions
 * from background threads (e.g., asset loaders or shader compilers) to be displayed on the UI thread.
 *
 * @todo Display time should be configurable, both as default and per log.
 */
class AnvilUILogger
{
private:
    static std::vector<UILogMessage> messages;
    static std::mutex queueMutex;

public:

    /**
     * @brief Thread-safe submission of a new log message to the on-screen overlay queue.
     *
     * @param inText The string text to display on screen.
     * @param inColor The text display color (defaults to AnvilColor::Green).
     */
    static void AddLog(const std::string& inText, ImVec4 inColor = AnvilColor::Green);

    /**
     * @brief Renders the active log queue inside an ImGui window overlay.
     * @warning Must be called once per frame inside an active ImGui context (between ImGui::NewFrame()
     * and ImGui::Render()).
     * @note Automatically decrements message timers and culls expired entries.
     */
    static void DrawOverlay();
};

/**
 * @brief Shorthand macro for submitting a message to the AnvilUILogger overlay.
 *
 * Syntactically mirrors AnvilUILogger::AddLog() and supports the following call signatures:
 * @code
 * LOGUI("Engine initialized successfully.");                   // Uses default green color
 * LOGUI("Failed to open shader file!", AnvilColor::Red);       // Explicit error color
 * @endcode
 *
 * @param ... Accepts a string text message and an optional ImVec4 color argument.
 * @see AnvilUILogger::AddLog
 */
#define LOGUI(...) AnvilUILogger::AddLog(__VA_ARGS__)

#endif //ANVIL_VK_UILOGGER_H

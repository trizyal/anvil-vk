// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SCREENLOGGER_H
#define ANVIL_VK_SCREENLOGGER_H

/**
 * @file ScreenLogger.h
 * @brief Thread-safe on-screen UI logger and ImGui overlay for runtime diagnostics.
 */

#include <string>
#include <vector>
#include <mutex>

#include "Color.h"

/**
 * @brief Internal container representing a single timed log notification.
 */
struct UILogMessage
{
    std::string text;    /**< Message string to display on screen. */
    Color color;         /**< RGBA color for rendering the text in ImGui. */
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
class ScreenLogger
{
private:
    static std::mutex queueMutex;

public:
    static std::vector<UILogMessage> messages;

    /**
     * @brief Thread-safe submission of a new log message to the on-screen overlay queue.
     *
     * @param inText The string text to display on screen.
     * @param inColor The text display color (defaults to Colors::Green).
     */
    static void AddLog(const std::string& inText, Color inColor = Colors::Green);

    /**
     * @brief Calulates and culls logs.
     * @note Automatically decrements message timers and culls expired entries.
     */
    static void UpdateAndGetMessages(float dt);
};

/**
 * @brief Shorthand macro for submitting a message to the ScreenLogger overlay.
 *
 * Syntactically mirrors ScreenLogger::AddLog() and supports the following call signatures:
 * @code
 * LOGUI("Engine initialized successfully.");                   // Uses default green color
 * LOGUI("Failed to open shader file!", AnvilColor::Red);       // Explicit error color
 * @endcode
 *
 * @param ... Accepts a string text message and an optional ImVec4 color argument.
 * @see ScreenLogger::AddLog
 */
#define LOGUI(...) ScreenLogger::AddLog(__VA_ARGS__)

#endif //ANVIL_VK_SCREENLOGGER_H

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_UILOGGER_H
#define ANVIL_VK_UILOGGER_H

#include <string>
#include <vector>
#include <mutex>

#include <imgui.h>

namespace AnvilColor {
    inline constexpr auto Green  = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Default
    inline constexpr auto Red    = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Errors
    inline constexpr auto Yellow = ImVec4(1.0f, 1.0f, 0.2f, 1.0f); // Warnings
    inline constexpr auto Blue   = ImVec4(0.3f, 0.7f, 1.0f, 1.0f); // Info
    inline constexpr auto White  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Neutral
    inline constexpr auto Gray   = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // Debug/Spam
}

struct UILogMessage
{
    std::string text;
    ImVec4 color;
    float timeRemaining;
};

class AnvilUILogger
{
private:
    static std::vector<UILogMessage> messages;
    static std::mutex queueMutex;

public:
    static void AddLog(const std::string& inText, ImVec4 inColor = AnvilColor::Green);
    static void DrawOverlay();
};

#define LOGUI(...) AnvilUILogger::AddLog(__VA_ARGS__)

#endif //ANVIL_VK_UILOGGER_H

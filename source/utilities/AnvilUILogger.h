// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_UILOGGER_H
#define ANVIL_VK_UILOGGER_H

#include <string>
#include <vector>
#include <mutex>

#include <imgui.h>

namespace AnvilColor {
    inline constexpr ImVec4 Green  = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Default
    inline constexpr ImVec4 Red    = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Errors
    inline constexpr ImVec4 Yellow = ImVec4(1.0f, 1.0f, 0.2f, 1.0f); // Warnings
    inline constexpr ImVec4 Blue   = ImVec4(0.3f, 0.7f, 1.0f, 1.0f); // Info
    inline constexpr ImVec4 White  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Neutral
    inline constexpr ImVec4 Gray   = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // Debug/Spam
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

// LOGUI(text, color)
#define LOGUI(...) AnvilUILogger::AddLog(__VA_ARGS__)

#endif //ANVIL_VK_UILOGGER_H

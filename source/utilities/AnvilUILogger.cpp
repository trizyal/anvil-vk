// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilUILogger.h"

std::vector<UILogMessage> AnvilUILogger::messages;
std::mutex AnvilUILogger::queueMutex;

constexpr float LOG_DISPLAY_TIME = 3.0f;

void AnvilUILogger::AddLog(const std::string& inText, ImVec4 inColor)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    messages.push_back({inText, inColor, LOG_DISPLAY_TIME});
}

void AnvilUILogger::DrawOverlay()
{
    std::lock_guard<std::mutex> lock(queueMutex);

    if (messages.empty())
    {
        return;
    }

    // Get the position of the main application window
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImVec2 overlayPosition = ImVec2(mainViewport->WorkPos.x + 10.f, mainViewport->WorkPos.y + 10.f);

    ImGuiWindowFlags loggerFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowPos(overlayPosition, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.7f);
}

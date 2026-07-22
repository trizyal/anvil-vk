// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilUILogger.h"

std::vector<UILogMessage> AnvilUILogger::messages;
std::mutex AnvilUILogger::queueMutex;

constexpr float LOG_DISPLAY_TIME = 5.0f;

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
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground;

    ImGui::SetNextWindowPos(overlayPosition, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (ImGui::Begin("AnvilLogOverlay", nullptr, loggerFlags))
    {
        float dt = ImGui::GetIO().DeltaTime;

        // Iterate backwards so we can safely erase items that expire
        for (int i = static_cast<int>(messages.size()) - 1; i >= 0; --i)
        {
            messages[i].timeRemaining -= dt;

            if (messages[i].timeRemaining <= 0.0f)
            {
                messages.erase(messages.begin() + i);
            }
            else
            {
                // Smooth fade out in the last second
                ImVec4 drawColor = messages[i].color;
                if (messages[i].timeRemaining < 1.0f)
                {
                    drawColor.w *= messages[i].timeRemaining;
                }

                // Draw the text
                ImGui::TextColored(drawColor, "%s", messages[i].text.c_str());
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

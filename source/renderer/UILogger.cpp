// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "UILogger.h"

std::vector<UILogMessage> UILogger::messages;
std::mutex UILogger::queueMutex;

void UILogger::AddLog(const std::string& inText, const ImVec4 inColor)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    messages.push_back({.text = inText, .color = inColor, .timeRemaining = LOG_DISPLAY_TIME});
}

void UILogger::DrawOverlay()
{
    std::lock_guard<std::mutex> lock(queueMutex);

    if (messages.empty())
    {
        return;
    }

    // Get the position of the main application window
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    const ImVec2 overlay_position = ImVec2(main_viewport->WorkPos.x + 10.f, main_viewport->WorkPos.y + 10.f);

    constexpr ImGuiWindowFlags logger_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground;

    ImGui::SetNextWindowPos(overlay_position, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (ImGui::Begin("AnvilLogOverlay", nullptr, logger_flags))
    {
        const float dt = ImGui::GetIO().DeltaTime;

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
                ImVec4 draw_color = messages[i].color;
                if (messages[i].timeRemaining < 1.0f)
                {
                    draw_color.w *= messages[i].timeRemaining;
                }

                // Draw the text
                ImGui::TextColored(draw_color, "%s", messages[i].text.c_str());
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

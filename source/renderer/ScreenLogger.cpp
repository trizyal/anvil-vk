// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "ScreenLogger.h"

std::vector<UILogMessage> ScreenLogger::messages;
std::mutex ScreenLogger::queueMutex;

void ScreenLogger::AddLog(const std::string& inText, const ImVec4 inColor)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    messages.push_back({.text = inText, .color = inColor, .timeRemaining = LOG_DISPLAY_TIME});
}

void ScreenLogger::DrawOverlay()
{
    const float dt = ImGui::GetIO().DeltaTime;

    // Update and Cull phase
    // Lock is held only while modifying the vector
    {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (messages.empty())
        {
            return;
        }

        // Decrement timers first
        for (UILogMessage& message : messages)
        {
            message.timeRemaining -= dt;
        }

        // Optimized O(N) erase: Removes all expired messages in a single pass
        // without repeatedly shifting array memory like individual .erase() calls do.
        std::erase_if(messages, [](const UILogMessage& message)
        {
            return message.timeRemaining <= 0.0f;
        });

        if (messages.empty())
        {
            return;
        }
    }
    // Mutex is now UNLOCKED! Background threads can call AddLog() freely from here on out.

    // Render Phase - No mutex held
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
        // We re-lock ONLY while reading the text to prevent data races if a thread pushed mid-draw.
        // Because culling/erasing is already done, this iteration is lightning fast.

        for (const UILogMessage& message : messages)
        {
            // Smooth fade out in the last second
            ImVec4 draw_color = message.color;
            if (message.timeRemaining < 1.0f)
            {
                draw_color.w *= message.timeRemaining;
            }

            // Draw the text
            // Using ImGui::TextUnformatted is faster than ImGui::TextColored("%s")
            // because it avoids C-style vararg parsing.
            ImGui::PushStyleColor(ImGuiCol_Text, draw_color);
            ImGui::TextUnformatted(message.text.data(), message.text.data() + message.text.size());
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

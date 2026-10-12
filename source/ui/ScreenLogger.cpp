// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "ScreenLogger.h"

#include <iostream>

#include "Logger.h"
#include "Trace.h"

std::vector<UILogMessage> ScreenLogger::messages;
std::mutex ScreenLogger::queueMutex;

void ScreenLogger::AddLog(const std::string& inText, const ImVec4 inColor)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    messages.push_back({.text = inText, .color = inColor, .timeRemaining = LOG_DISPLAY_TIME});
}

void ScreenLogger::UpdateAndGetMessages(const float dt)
{
    SCOPE_CPU;

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
}

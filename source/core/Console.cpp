// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Console.h"

#ifdef _WIN32
#   include <windows.h>
#   include <shellapi.h>
#elif defined(__linux__)
#   include <fstream>
#endif

#include <sstream>

#include "Ensure.h"
#include "Logger.h"
#include "stox.h"
#include "Trace.h"

/**
 * @brief Internal flag tracking whether the console UI should scroll to the bottom of the log.
 */
static bool s_ScrollToBottom = false;

std::unordered_map<std::string, CVar>& Console::GetCVars()
{
    static std::unordered_map<std::string, CVar> s_CVars;
    return s_CVars;
}

std::unordered_map<std::string, std::pair<std::string, Console::CommandCallback>>& Console::GetCommands()
{
    static std::unordered_map<std::string, std::pair<std::string, CommandCallback>> s_Commands;
    return s_Commands;
}

std::vector<std::string>& Console::GetLogHistory()
{
    static std::vector<std::string> s_LogHistory;
    return s_LogHistory;
}

std::vector<std::string>& Console::GetCommandHistory()
{
    static std::vector<std::string> s_CommandHistory;
    return s_CommandHistory;
}

void Console::SetDefaultCommands()
{
    SCOPE_CPU;
    LOG_TRACE("Initialize Console.");

    RegisterCommand("clear", "Clears the console log history.", [](const std::vector<std::string>&)
    {
        ClearLog();
    });

    RegisterCommand("help", "Lists all registered commands and CVars.", [](const std::vector<std::string>&)
    {
        Print("--- Commands ---");
        for (const auto& [name, data] : GetCommands())
        {
            Print(name + " - " + data.first);
        }

        Print("--- CVars ---");
        for (const auto& [name, cvar] : GetCVars())
        {
            Print(name + " - " + cvar.description);
        }
    });

    Print("Anvil Console Initialized. Type 'help' for commands.");
}

void Console::RegisterCVarInt(const std::string& name, const std::string& description, int defaultValue, const CVarFlags flags)
{
    LOG_DEBUG("Registering CVar Int: {} {}", name, defaultValue);
    GetCVars()[name] = {.type = CVarType::Int, .flags = flags, .description = description, .value = defaultValue};
}

void Console::RegisterCVarFloat(const std::string& name, const std::string& description, float defaultValue, const CVarFlags flags)
{
    LOG_DEBUG("Registering CVar Float: {} {}", name, defaultValue);
    GetCVars()[name] = {.type = CVarType::Float, .flags = flags, .description = description, .value = defaultValue};
}

void Console::RegisterCVarBool(const std::string& name, const std::string& description, bool defaultValue, const CVarFlags flags)
{
    LOG_DEBUG("Registering CVar Bool: {} {}", name, defaultValue);
    GetCVars()[name] = {.type = CVarType::Bool, .flags = flags, .description = description, .value = defaultValue};
}

void Console::RegisterCVarString(const std::string& name, const std::string& description, std::string& defaultValue, const CVarFlags flags)
{
    LOG_DEBUG("Registering CVar String: {} {}", name, defaultValue);
    GetCVars()[name] = {.type = CVarType::String, .flags = flags, .description = description, .value = defaultValue};
}

int Console::GetCVarInt(const std::string& name)
{
    if (GetCVars().contains(name))
    {
        return std::get<int>(GetCVars()[name].value);
    }
    LOG_WARN("CVar Int {} does not exist.", name);
    return 0;
}

float Console::GetCVarFloat(const std::string& name)
{
    if (GetCVars().contains(name))
    {
        return std::get<float>(GetCVars()[name].value);
    }
    LOG_WARN("CVar Float {} does not exist.", name);
    return 0.0f;
}

bool Console::GetCVarBool(const std::string& name)
{
    if (GetCVars().contains(name))
    {
        return std::get<bool>(GetCVars()[name].value);
    }
    LOG_WARN("CVar Bool {} does not exist.", name);
    return false;
}

std::string Console::GetCVarString(const std::string& name)
{
    if (GetCVars().contains(name))
    {
        return std::get<std::string>(GetCVars()[name].value);
    }
    LOG_WARN("CVar String {} does not exist.", name);
    return "";
}

void Console::SetCVarInt(const std::string& name, int value)
{
    if (GetCVars().contains(name) && GetCVars()[name].type == CVarType::Int)
    {
        if (GetCVars()[name].flags != CVarFlags::None)
        {
            LOG_WARN("CVar Int {} cannot be changed in runtime.", name);
        }

        LOG_DEBUG("Updating CVar Int: {} {}", name, value);
        GetCVars()[name].value = value;
        return;
    }
    LOG_WARN("CVar Int {} does not exist.", name);
}

void Console::SetCVarFloat(const std::string& name, float value)
{
    if (GetCVars().contains(name) && GetCVars()[name].type == CVarType::Float)
    {
        if (GetCVars()[name].flags != CVarFlags::None)
        {
            LOG_WARN("CVar Float {} cannot be changed in runtime.", name);
        }

        LOG_DEBUG("Updating CVar Float: {} {}", name, value);
        GetCVars()[name].value = value;
        return;
    }
    LOG_WARN("CVar Float {} does not exist.", name);
}

void Console::SetCVarBool(const std::string& name, bool value)
{
    if (GetCVars().contains(name) && GetCVars()[name].type == CVarType::Bool)
    {
        if (GetCVars()[name].flags != CVarFlags::None)
        {
            LOG_WARN("CVar Bool {} cannot be changed in runtime.", name);
        }

        LOG_DEBUG("Updating CVar Bool: {} {}", name, value);
        GetCVars()[name].value = value;
        return;
    }
    LOG_WARN("CVar Bool {} does not exist.", name);
}

void Console::SetCVarString(const std::string& name, std::string& value)
{
    if (GetCVars().contains(name) && GetCVars()[name].type == CVarType::String)
    {
        if (GetCVars()[name].flags != CVarFlags::None)
        {
            LOG_WARN("CVar String {} cannot be changed in runtime.", name);
        }

        LOG_DEBUG("Updating CVar String: {} {}", name, value);
        GetCVars()[name].value = value;
        return;
    }
    LOG_WARN("CVar String {} does not exist.", name);
}

void Console::RegisterCommand(const std::string& name, const std::string& description, CommandCallback callback)
{
    LOG_DEBUG("Registering Command: {}", name);
    GetCommands()[name] = {description, callback};
}

void Console::Print(const std::string& message)
{
    GetLogHistory().push_back(message);
    s_ScrollToBottom = true;
}

void Console::ClearLog()
{
    LOG_DEBUG("Clear Console Log.");
    GetLogHistory().clear();
}

bool Console::ShouldScroll()
{
    return s_ScrollToBottom;
}

void Console::ClearScroll()
{
    s_ScrollToBottom = false;
}

void Console::Execute(const std::string& commandLine)
{
    SCOPE_CPU;
    LOG_TRACE("Executing command: {}", commandLine);

    Print("] " + commandLine);

    // Save to command history, preventing consecutive duplicates
    if (!commandLine.empty())
    {
        auto& history = GetCommandHistory();
        if (history.empty() || history.back() != commandLine)
        {
            history.push_back(commandLine);
        }
    }

    std::istringstream stream(commandLine);
    std::string token;
    std::vector<std::string> args;

    while (stream >> token)
    {
        args.push_back(token);
    }

    if (args.empty())
    {
        return;
    }

    std::string cmdName = args[0];
    args.erase(args.begin());

    // FIX: Discard the '=' sign if used typed it with spaces (e.g. cvar = 100)
    if (!args.empty() && args[0] == "=")
    {
        args.erase(args.begin());
    }

    if (GetCommands().contains(cmdName))
    {
        GetCommands()[cmdName].second(args);
        return;
    }

    if (GetCVars().contains(cmdName))
    {
        CVar& cvar = GetCVars()[cmdName];

        if (args.empty())
        {
            std::visit([&](const auto& activeValue)
            {
                Print(std::format("{} = {}", cmdName, activeValue));
                LOG_INFO("CVar: {} = {}", cmdName, activeValue);
            }, cvar.value);

            return;
        }

        // Block runtime modification of InitOnly variables
        if (!bIsBooting && (cvar.flags & CVarFlags::InitOnly))
        {
            Print("Error: '" + cmdName + "' is an initialization parameter and cannot be changed at runtime.");
            LOG_INFO("CVar {} is an initialization parameter and cannot be changed at runtime.", cmdName);
            return;
        }

        // Block modification of ReadOnly variables
        if (cvar.flags & CVarFlags::ReadOnly)
        {
            Print("Error: '" + cmdName + "' is a read-only parameter and cannot be changed.");
            LOG_INFO("CVar {} is  a read-only parameter and cannot be changed.", cmdName);
            return;
        }

        bool b_success = false;
        switch (cvar.type)
        {
        case CVarType::Int:
            if (auto ret = tml::SToInt(args[0]))
            {
                cvar.value = ret.value();
                b_success = true;
            }
            break;

        case CVarType::Float:
            if (auto ret = tml::SToFloat(args[0]))
            {
                cvar.value = ret.value();
                b_success = true;
            }
            break;

        case CVarType::Bool:
            cvar.value = (args[0] == "1" || args[0] == "true");
            b_success = true;
            break;

        case CVarType::String:
            cvar.value = args[0];
            b_success = true;
            break;
        }

        if (b_success)
        {
            std::visit([&](const auto& activeValue)
            {
                Print(std::format("{} updated to {}", cmdName, activeValue));
                LOG_INFO("CVar {} updated to {}", cmdName, activeValue);
            }, cvar.value);
        }
        else
        {
            Print("Error: Invalid argument format.");
            LOG_INFO("Error: Invalid argument format.");
        }
        return;
    }

    Print("Unknown command or CVar: " + cmdName);
}

void Console::ParseCommandLine()
{

}

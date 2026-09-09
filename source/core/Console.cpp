// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Console.h"
#include <sstream>

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

void Console::Initialize()
{
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

void Console::RegisterCVarInt(const std::string& name, const std::string& description, int defaultValue)
{
    GetCVars()[name] = {.type = CVarType::Int, .description = description, .value = defaultValue};
}

void Console::RegisterCVarFloat(const std::string& name, const std::string& description, float defaultValue)
{
    GetCVars()[name] = {.type = CVarType::Float, .description = description, .value = defaultValue};
}

void Console::RegisterCVarBool(const std::string& name, const std::string& description, bool defaultValue)
{
    GetCVars()[name] = {.type = CVarType::Bool, .description = description, .value = defaultValue};
}

int Console::GetCVarInt(const std::string& name)
{
    return std::get<int>(GetCVars()[name].value);
}

float Console::GetCVarFloat(const std::string& name)
{
    return std::get<float>(GetCVars()[name].value);
}

bool Console::GetCVarBool(const std::string& name)
{
    return std::get<bool>(GetCVars()[name].value);
}

void Console::RegisterCommand(const std::string& name, const std::string& description, CommandCallback callback)
{
    GetCommands()[name] = {description, callback};
}

void Console::Print(const std::string& message)
{
    GetLogHistory().push_back(message);
    s_ScrollToBottom = true;
}

void Console::ClearLog()
{
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
    Print("] " + commandLine);

    std::istringstream stream(commandLine);
    std::string token;
    std::vector<std::string> args;

    while (stream >> token) args.push_back(token);
    if (args.empty()) return;

    std::string cmdName = args[0];
    args.erase(args.begin());

    if (GetCommands().contains(cmdName)) {
        GetCommands()[cmdName].second(args);
        return;
    }

    if (GetCVars().contains(cmdName)) {
        CVar& cvar = GetCVars()[cmdName];

        if (args.empty())
        {
            if (cvar.type == CVarType::Int)
            {
                Print(cmdName + " = " + std::to_string(std::get<int>(cvar.value)));
            }
            else if (cvar.type == CVarType::Float)
            {
                Print(cmdName + " = " + std::to_string(std::get<float>(cvar.value)));
            }
            else if (cvar.type == CVarType::Bool)
            {
                Print(cmdName + " = " + (std::get<bool>(cvar.value) ? "true" : "false"));
            }
            return;
        }

        try {
            if (cvar.type == CVarType::Int)
            {
                cvar.value = std::stoi(args[0]);
            }
            else if (cvar.type == CVarType::Float)
            {
                cvar.value = std::stof(args[0]);
            }
            else if (cvar.type == CVarType::Bool)
            {
                cvar.value = (args[0] == "1" || args[0] == "true");
            }
            Print(cmdName + " updated.");
        } catch (...) {
            Print("Error: Invalid argument format.");
        }
        return;
    }
    Print("Unknown command or CVar: " + cmdName);
}

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_CONSOLE_H
#define ANVIL_VK_CONSOLE_H

/**
 * @file Console.h
 * @brief Developer console managing global CVars (Console Variables) and executable commands.
 */

#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>

/**
 * @brief Defines the underlying data type of a Console Variable.
 */
enum class CVarType
{
    Int, /**< Integer */
    Float, /**< Float */
    Bool, /**< Boolean */
    String /**< String */
};

/**
 * @brief Holds the metadata and current runtime value of a registered Console Variable.
 */
struct CVar
{
    CVarType type;
    std::string description;
    std::variant<int, float, bool, std::string> value;
};

/**
 * @brief Global registry for engine CVars and Commands.
 *
 * Utilizes Meyer's Singleton pattern internally to guarantee safe initialization
 * order for globally registered variables prior to main() execution.
 */
class Console
{
    public:
    /** @brief Signature for console command callbacks, accepting a list of string arguments. */
    using CommandCallback = std::function<void(const std::vector<std::string>&)>;

    /**
     * @brief Boots the console and registers core inbuilt commands (e.g., help, clear).
     */
    static void Initialize();

    /**
     * @brief Safely retrieves the global CVar registry.
     * @return Reference to the internal unordered_map of CVars.
     */
    static std::unordered_map<std::string, CVar>& GetCVars();

    /**
     * @brief Safely retrieves the global command registry.
     * @return Reference to the internal unordered_map of commands.
     */
    static std::unordered_map<std::string, std::pair<std::string, CommandCallback>>& GetCommands();

    /**
     * @brief Safely retrieves the console log history.
     * @return Reference to the vector of string logs.
     */
    static std::vector<std::string>& GetLogHistory();

    /**
     * @brief Safely retrieves the history of entered commands.
     * @return Reference to the vector of previously executed command strings.
     */
    static std::vector<std::string>& GetCommandHistory();

    /**
     * @brief Registers a new integer Console Variable.
     * @param name The unique identifier used to call this CVar from the console.
     * @param description Helpful text explaining what this variable controls.
     * @param defaultValue The initial value of the variable.
     */
    static void RegisterCVarInt(const std::string& name, const std::string& description, int defaultValue);

    /**
     * @brief Registers a new float Console Variable.
     * @param name The unique identifier used to call this CVar from the console.
     * @param description Helpful text explaining what this variable controls.
     * @param defaultValue The initial value of the variable.
     */
    static void RegisterCVarFloat(const std::string& name, const std::string& description, float defaultValue);

    /**
     * @brief Registers a new boolean Console Variable.
     * @param name The unique identifier used to call this CVar from the console.
     * @param description Helpful text explaining what this variable controls.
     * @param defaultValue The initial value of the variable.
     */
    static void RegisterCVarBool(const std::string& name, const std::string& description, bool defaultValue);

    /**
     * @brief Retrieves the current value of an integer CVar.
     * @param name The unique identifier of the CVar.
     * @return The current integer value, or 0 if not found.
     */
    static int GetCVarInt(const std::string& name);

    /**
     * @brief Retrieves the current value of a float CVar.
     * @param name The unique identifier of the CVar.
     * @return The current float value, or 0.0f if not found.
     */
    static float GetCVarFloat(const std::string& name);

    /**
     * @brief Retrieves the current value of a boolean CVar.
     * @param name The unique identifier of the CVar.
     * @return The current boolean value, or false if not found.
     */
    static bool GetCVarBool(const std::string& name);

    /**
     * @brief Updates the value of an existing integer CVar.
     * @param name The unique identifier of the CVar.
     * @param value The new integer value to set.
     */
    static void SetCVarInt(const std::string& name, int value);

    /**
     * @brief Updates the value of an existing float CVar.
     * @param name The unique identifier of the CVar.
     * @param value The new float value to set.
     */
    static void SetCVarFloat(const std::string& name, float value);

    /**
     * @brief Updates the value of an existing boolean CVar.
     * @param name The unique identifier of the CVar.
     * @param value The new boolean value to set.
     */
    static void SetCVarBool(const std::string& name, bool value);

    /**
     * @brief Registers a new executable command.
     * @param name The string typed into the console to trigger this command.
     * @param description Helpful text shown in the help menu.
     * @param callback The function executed when the command is invoked.
     */
    static void RegisterCommand(const std::string& name, const std::string& description, CommandCallback callback);

    /**
     * @brief Parses and executes a raw string inputted from the console UI.
     * @param commandLine The raw string to parse into command/CVar lookups and arguments.
     */
    static void Execute(const std::string& commandLine);

    /**
     * @brief Pushes a new text line to the console's visual log.
     * @param message The text to display.
     */
    static void Print(const std::string& message);

    /**
     * @brief Clears all text currently stored in the console log.
     */
    static void ClearLog();

    /**
     * @brief Checks if the UI needs to automatically scroll to the bottom.
     * @return True if a new message was recently added.
     */
    static bool ShouldScroll();

    /**
     * @brief Resets the scroll flag after the UI has processed it.
     */
    static void ClearScroll();
};

// Auto-Registration Macros

/**
 * @brief Helper struct that automatically registers a CVar upon instantiation.
 */
struct AutoRegisterCVar
{
    AutoRegisterCVar(const std::string& name, const std::string& description, int defaultValue)
    {
        Console::RegisterCVarInt(name, description, defaultValue);
    }

    AutoRegisterCVar(const std::string& name, const std::string& description, float defaultValue)
    {
        Console::RegisterCVarFloat(name, description, defaultValue);
    }

    AutoRegisterCVar(const std::string& name, const std::string& description, bool defaultValue)
    {
        Console::RegisterCVarBool(name, description, defaultValue);
    }
};

/**
 * @brief Helper struct that automatically registers a Command upon instantiation.
 */
struct AutoRegisterCommand
{
    AutoRegisterCommand(const std::string& name, const std::string& description, Console::CommandCallback callback)
    {
        Console::RegisterCommand(name, description, std::move(callback));
    }
};

/**
 * @brief Internal macro for resolving line numbers in macro expansion.
 */
#define CONCAT_IMPL(x, y) x##y

/**
 * @brief Internal macro for concatenating auto-generated variable names.
 */
#define CONCAT(x, y) CONCAT_IMPL(x, y)

/**
 * @brief Macro to define a global integer CVar anywhere in the codebase.
 */
#define CVAR_INT(name, description, defaultValue) \
    static AutoRegisterCVar CONCAT(auto_cvar_, __LINE__)(name, description, static_cast<int>(defaultValue))

/**
 * @brief Macro to define a global float CVar anywhere in the codebase.
 */
#define CVAR_FLOAT(name, description, defaultValue) \
    static AutoRegisterCVar CONCAT(auto_cvar_, __LINE__)(name, description, static_cast<float>(defaultValue))

/**
 * @brief Macro to define a global boolean CVar anywhere in the codebase.
 */
#define CVAR_BOOL(name, description, defaultValue) \
    static AutoRegisterCVar CONCAT(auto_cvar_, __LINE__)(name, description, static_cast<bool>(defaultValue))

/**
 * @brief Macro to define a global Console Command anywhere in the codebase.
 */
#define COMMAND(name, description, callback) \
    static AutoRegisterCommand CONCAT(auto_cmd_, __LINE__)(name, description, callback)

#endif //ANVIL_VK_CONSOLE_H

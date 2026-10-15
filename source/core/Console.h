// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

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
    String, /**< String */
};

/**
 * @brief Defines the underlying edit mode of a Console Variable.
 */
enum class CVarFlags : uint8_t
{
    None    = 0,
    InitOnly    = 1 << 0, /**< Cannot be changed via the in-game console after boot. */
    ReadOnly    = 1 << 1,  /**< Can only be read, never manually set. */
};

inline CVarFlags operator|(CVarFlags lhs, CVarFlags rhs)
{
    return static_cast<CVarFlags>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

inline bool operator&(CVarFlags lhs, CVarFlags rhs)
{
    return (static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs)) != 0;
}

/**
 * @brief Holds the metadata and current runtime value of a registered Console Variable.
 */
struct CVar
{
    CVarType type;
    CVarFlags flags;
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

    /** @brief Tracks if the engine is currently booting, allowing InitOnly CVars to be modified. */
    static inline bool bIsBooting = true;

    /**
     * @brief Boots the console and registers core inbuilt commands (e.g., help, clear).
     */
    static void InitializeConsole();

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
     * @param flags Edit scope for the cvar.
     */
    static void RegisterCVarInt(const std::string& name, const std::string& description, int defaultValue, CVarFlags flags = CVarFlags::None);

    /**
     * @brief Registers a new float Console Variable.
     * @param name The unique identifier used to call this CVar from the console.
     * @param description Helpful text explaining what this variable controls.
     * @param defaultValue The initial value of the variable.
     * @param flags Edit scope for the cvar.
     */
    static void RegisterCVarFloat(const std::string& name, const std::string& description, float defaultValue, CVarFlags flags = CVarFlags::None);

    /**
     * @brief Registers a new boolean Console Variable.
     * @param name The unique identifier used to call this CVar from the console.
     * @param description Helpful text explaining what this variable controls.
     * @param defaultValue The initial value of the variable.
     * @param flags Edit scope for the cvar.
     */
    static void RegisterCVarBool(const std::string& name, const std::string& description, bool defaultValue, CVarFlags flags = CVarFlags::None);

    /**
     * @brief Registers a new string Console Variable.
     * @param name The unique identifier used to call this CVar from the console.
     * @param description Helpful text explaining what this variable controls.
     * @param defaultValue The initial value of the variable.
     * @param flags Edit scope for the cvar.
     */
    static void RegisterCVarString(const std::string& name, const std::string& description, std::string& defaultValue, CVarFlags flags = CVarFlags::None);

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
     * @brief Retrieves the current value of a boolean CVar.
     * @param name The unique identifier of the CVar.
     * @return The current string, or "" if not found.
     */
    static std::string GetCVarString(const std::string& name);

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
     * @brief Updates the value of an existing string CVar.
     * @param name The unique identifier of the CVar.
     * @param value The new string value to set.
     */
    static void SetCVarString(const std::string& name, std::string& value);

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
    AutoRegisterCVar(const std::string& name, const std::string& description, int defaultValue, CVarFlags flags = CVarFlags::None)
    {
        Console::RegisterCVarInt(name, description, defaultValue, flags);
    }

    AutoRegisterCVar(const std::string& name, const std::string& description, float defaultValue, CVarFlags flags = CVarFlags::None)
    {
        Console::RegisterCVarFloat(name, description, defaultValue, flags);
    }

    AutoRegisterCVar(const std::string& name, const std::string& description, bool defaultValue, CVarFlags flags = CVarFlags::None)
    {
        Console::RegisterCVarBool(name, description, defaultValue, flags);
    }

    AutoRegisterCVar(const std::string& name, const std::string& description, std::string& defaultValue, CVarFlags flags = CVarFlags::None)
    {
        Console::RegisterCVarString(name, description, defaultValue, flags);
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
 * @brief Macro to define a global string CVar anywhere in the codebase.
 */
#define CVAR_STRING(name, description, defaultValue) \
    static AutoRegisterCVar CONCAT(auto_cvar_, __LINE__)(name, description, static_cast<std::string>(defaultValue))

/**
 * @brief Macros to define global Int CVars with explicit CVarFlags.
 */
#define CVAR_INT_FLAG(name, description, defaultValue, flags) \
    static AutoRegisterCVar CONCAT(auto_cvar_, __LINE__)(name, description, static_cast<int>(defaultValue), flags)

/**
 * @brief Macros to define global Float CVars with explicit CVarFlags.
 */
#define CVAR_FLOAT_FLAG(name, description, defaultValue, flags) \
    static AutoRegisterCVar CONCAT(auto_cvar_, __LINE__)(name, description, static_cast<float>(defaultValue), flags)

/**
 * @brief Macros to define global Bool CVars with explicit CVarFlags.
 */
#define CVAR_BOOL_FLAG(name, description, defaultValue, flags) \
    static AutoRegisterCVar CONCAT(auto_cvar_, __LINE__)(name, description, static_cast<bool>(defaultValue), flags)

/**
 * @brief Macros to define global String CVars with explicit CVarFlags.
 */
#define CVAR_STRING_FLAG(name, description, defaultValue, flags) \
    static AutoRegisterCVar CONCAT(auto_cvar_, __LINE__)(name, description, defaultValue, flags)

/**
 * @brief Macro to define a global Console Command anywhere in the codebase.
 */
#define COMMAND(name, description, callback) \
    static AutoRegisterCommand CONCAT(auto_cmd_, __LINE__)(name, description, callback)

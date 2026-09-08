// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_CONSOLE_H
#define ANVIL_VK_CONSOLE_H

/**
 * @file Console.h
 * @brief Developer console managing global CVars (Console Variables) and executable commands.
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>

/**
 * @brief Defines the underlying data type of a Console Variable.
 */
enum class CVarType
{
    I, /**< Integer */
    F, /**< Float */
    B, /**< Boolean */
    S /**< String */
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
    /** Signature for console command callbacks, accepting a list of string arguments. */
    using CommandCallback = std::function<void(const std::vector<std::string>&)>;

    /**
     * @brief Boots the console and registers core inbuilt commands.
     */
    static void Initialize();
};

#endif //ANVIL_VK_CONSOLE_H

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef STOX_H
#define STOX_H

/**
 * @file stox.h
 * @brief Wrappers around std::stoi and std::stof to omit try{} catch(...){} block.
 *
 * @see https://en.cppreference.com/cpp/string/basic_string/stol
 * @see https://en.cppreference.com/cpp/string/basic_string/stof
 */

#include <optional>
#include <string>

namespace tml
{
    /**
     * @brief Safely parses a string into an integer.
     * @param str The string to parse.
     * @return The parsed integer, or std::nullopt if parsing fails.
     */
    std::optional<int> SToInt(const std::string& str);

    /**
     * @brief Safely parses a string into a float.
     * @param str The string to parse.
     * @return The parsed float, or std::nullopt if parsing fails.
     */
    std::optional<float> SToFloat(const std::string& str);
}

#endif //STOX_H

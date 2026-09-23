// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_ASSERT_H
#define ANVIL_VK_ASSERT_H

/**
 * @file Ensure.h
 * @brief System-wide assertion macros for handling fatal and non-fatal engine errors.
 */

#include <iostream>
#include <cstdlib>

/**
 * @brief Platform-specific intrinsic to trigger a debugger breakpoint.
 *
 * Halts execution if a debugger is attached. If no debugger is attached,
 * behavior depends on the OS (often terminates the program or ignores).
 */
#if defined(_MSC_VER)
    #define BREAK() __debugbreak()
#else
    #define BREAK() __builtin_trap()
#endif

/**
 * @brief Soft assertion: Logs an error and triggers a breakpoint, but continues execution.
 *
 * Use this for recoverable errors where you want to notify the developer but
 * avoid crashing the application.
 *
 * @param condition The expression that is expected to evaluate to true.
 * @param message The diagnostic message to print if the condition is false.
 */
#define ENSURE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "[Ensure Failed] " << message << "\n" \
                      << "File: " << __FILE__ << ":" << __LINE__ << std::endl; \
            BREAK(); \
        } \
    } while (false)

/**
 * @brief Fatal assertion: Logs an error, triggers a breakpoint, and crashes the app.
 *
 * Use this for unrecoverable errors (like missing critical assets) where continuing
 * execution would lead to undefined behavior or immediate segmentation faults.
 *
 * @param condition The expression that is expected to evaluate to true.
 * @param message The diagnostic message to print if the condition is false.
 */
#define FATAL(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "[Fatal Error] " << message << "\n" \
                      << "File: " << __FILE__ << ":" << __LINE__ << std::endl; \
            BREAK(); \
            std::abort(); \
        } \
    } while (false)

#endif //ANVIL_VK_ENSURE_H

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef LOGGER_H
#define LOGGER_H


/**
 * @file Logger.h
 */

#include <string>
#include <fstream>
#include <cstdint>
#include <source_location>
#include <mutex>
#include <format>

/**
 * @brief Defines the severity of a log message.
 */
enum class LogLevel : uint8_t
{
    Fatal   = 0,
    Error   = 1,
    Warning = 2,
    Info    = 3,
    Debug   = 4,
    Trace   = 5
};

/**
 * @brief Centralized thread-safe logging subsystem.
 *
 * Intercepts engine and application logs, formats them with timestamps and source locations,
 * filters them against the runtime log verbosity CVar, and routes them to the terminal,
 * a log file, and the in-game developer console.
 */
class Logger
{
public:
    /**
     * @brief Boots the logging system and opens the output file.
     * @param logFilePath Path to the desired output .log file.
     */
    static void InitializeLogger(const std::string& logFilePath);

    /**
     * @brief Safely flushes and closes the output log file.
     */
    static void ShutdownLogger();

    /**
     * @brief Central routing function that processes the finalized log string.
     * @param level Severity of the log.
     * @param location Captured source location (file, line).
     * @param message The fully formatted string to log.
     */
    static void LogMessage(LogLevel level, const std::source_location& location, const std::string& message);

    /**
     * @brief Variadic template supporting std::format syntax for dynamic arguments.
     * @tparam Args
     * @param level Severity of the log.
     * @param location Captured source location (file, line).
     * @param fmt The format string.
     * @param args Dynamic arguments to format into the string.
     */
    template <typename... Args>
    static void Log(LogLevel level, const std::source_location& location, std::format_string<Args...> fmt, Args&&... args)
    {
        LogMessage(level, location, std::format(fmt, std::forward<Args>(args)...));
    }

private:
    static std::ofstream s_FileStream;
    static std::mutex s_LogMutex;

    static std::string GetTimestamp();
    static const char* LevelToString(LogLevel level);
    static int GetMaxVerbosity();
};

#define LOG_FATAL(...) Logger::Log(LogLevel::Fatal, std::source_location::current(), __VA_ARGS__)
#define LOG_ERROR(...) Logger::Log(LogLevel::Error, std::source_location::current(), __VA_ARGS__)
#define LOG_WARN(...) Logger::Log(LogLevel::Warning, std::source_location::current(), __VA_ARGS__)
#define LOG_INFO(...) Logger::Log(LogLevel::Info, std::source_location::current(), __VA_ARGS__)
#define LOG_DEBUG(...) Logger::Log(LogLevel::Debug, std::source_location::current(), __VA_ARGS__)
#define LOG_TRACE(...) Logger::Log(LogLevel::Trace, std::source_location::current(), __VA_ARGS__)

#endif //LOGGER_H

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Logger.h"

#include <chrono>
#include <filesystem>

#include "Console.h"
#include "Ensure.h"

// Initialize static members
std::ofstream Logger::s_FileStream;
std::mutex Logger::s_LogMutex;

void Logger::InitializeLogger(const std::string& logFilePath)
{
    // 1. Ensure the target directory exists before creating the file
    std::filesystem::path path(logFilePath);
    if (path.has_parent_path())
    {
        if (!std::filesystem::create_directories(path.parent_path()))
        {
            LOG_ERROR("Failed to create log directory: {}", path.parent_path().string());
        }
        else
        {
            LOG_INFO("Created log directory: {}", path.parent_path().string());
        }
    }

    // Open the log file, truncating any previous run's data
    s_FileStream.open(logFilePath, std::ios::out | std::ios::trunc);
    if (!s_FileStream.is_open())
    {
        // If the log file doesn't open
        // logging is done just in the terminal
        // It should not change any of the usecase though
        LOG_ERROR("Failed to open log file: {}.", logFilePath);
    }
    else
    {
        LOG_INFO("Open log file: {}", logFilePath);
    }

    LOG_INFO("Logger initialized.");
}

void Logger::ShutdownLogger()
{
    LOG_TRACE("Logger shutting down.");

    if (s_FileStream.is_open())
    {
        s_FileStream.close();
    }
}

int Logger::GetMaxVerbosity()
{
    if (Console::GetCVars().contains("a.logverbosity"))
    {
        return Console::GetCVarInt("a.logverbosity");
    }

    // Should never get here.
    ENSURE(false, "Reached unexpected scope.");
    return 3;
}

const char* Logger::LevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Fatal:   return "FATAL";
    case LogLevel::Error:   return "ERROR";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Info:    return "INFO";
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Trace:   return "TRACE";
    }

    // Should never get here.
    ENSURE(false, "Reached unexpected scope.");
    return "UNKNOWN";
}

std::string Logger::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    return std::format("{:%Y-%m-%d %H:%M:%S}", now);
}

void Logger::LogMessage(LogLevel level, const std::source_location& location, const std::string& message)
{
    // Filter by CVar verbosity
    int verbosity = GetMaxVerbosity();
    int level_int = static_cast<int>(level);

    if (level_int > verbosity)
    {
        return;
    }

    // Extract the short filename from the full source path
    std::string filename = location.file_name();
    filename = filename.substr(filename.find_last_of("/\\") + 1);

    // Format the final output string
    // Format: [YYYY-MM-DD HH:MM:SS] [INFO] [File.cpp:42] Log message
    std::string formatted_log = std::format("[{}] [{}] [{}:{}] {}",
        GetTimestamp(),
        LevelToString(level),
        filename,
        location.line(),
        message
    );

    // Lock the mutex so threads don't scramble standard out or file writes
    std::lock_guard lock(s_LogMutex);

    // Terminal output
    if (level == LogLevel::Fatal || level == LogLevel::Error)
    {
        std::cerr << formatted_log << std::endl;
    }
    else
    {
        std::cout << formatted_log << std::endl;
    }

    // File output
    if (s_FileStream.is_open())
    {
        s_FileStream << formatted_log << std::endl;
        s_FileStream.flush(); // Ensure it writes immediately in case of a hard crash
    }

    // In-Game developer console output
#if UNIMPLEMENTED
    // We omit the timestamp for the in-game console to keep it cleaner
    std::string consoleLog = std::format("[{}] {}", LevelToString(level), message);
    Console::Print(consoleLog);
#endif //UNIMPLEMENTED
}

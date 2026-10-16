// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Logger.h"

#include <chrono>
#include <filesystem>

#include "Console.h"
#include "Ensure.h"
#include "Trace.h"

CVAR_INT("e.logverbosity",  5, CVarFlags::None,
    "0: Fatal, 1: Error, 2: Warning, 3: Info, 4: Debug, 5: Trace");

CVAR_STRING("sys.logdir", "/saved/logs", CVarFlags::InitOnly,
    "Logs Directory");

// Initialize static members
std::ofstream Logger::s_FileStream;
std::mutex Logger::s_LogMutex;

void Logger::InitializeLogger(const std::string& logDirectory)
{
    SCOPE_CPU;
    
    // Ensure the target directory exists before creating the file
    std::filesystem::path path(ENGINE_DIR + logDirectory);
    std::error_code ec;

    if (std::filesystem::exists(path))
    {
        // It already exists. We can log this if we want, or just stay silent.
        LOG_INFO("Log directory already exists: {}", path.string());
    }
    else if (std::filesystem::create_directories(path, ec))
    {
        LOG_INFO("Created log directory: {}", path.string());
    }
    else
    {
        // ec.message() will tell us exactly why it failed (e.g., "Permission denied")
        LOG_ERROR("Failed to create log directory: {} - {}", path.string(), ec.message());
    }

    // Generate the timestamped filename (e.g., log2026-10-16-1336.txt)
    const auto now = std::chrono::system_clock::now();
    // Truncate to minutes to ensure clean formatting
    auto now_mins = std::chrono::floor<std::chrono::minutes>(now);
    const std::string file_name = std::format("log{:%Y-%m-%d-%H%M}.txt", now_mins);

    // Combine the directory path with the generated filename
    std::filesystem::path file_path = path / file_name;

    // Open the log file, truncating any previous run's data
    s_FileStream.open(file_path.string(), std::ios::out | std::ios::trunc);
    if (!s_FileStream.is_open())
    {
        // If the log file doesn't open
        // logging is done just in the terminal
        // It should not change any of the usecase though
        LOG_ERROR("Failed to open log file: {}.", file_path.string());
    }
    else
    {
        LOG_INFO("Open log file: {}", file_path.string());
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
    if (Console::GetCVars().contains("e.logverbosity"))
    {
        return Console::GetCVarInt("e.logverbosity");
    }

    return 5;
}

std::string Logger::LevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Fatal:   return "FATAL";
    case LogLevel::Error:   return "ERROR";
    case LogLevel::Warning: return "WARN ";
    case LogLevel::Info:    return "INFO ";
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Trace:   return "TRACE";
    }

    // Should never get here.
    ENSURE(false, "Reached unexpected scope.");
    return "UNKNOWN";
}

// Helper to retrieve the correct engine color per log level
Color Logger::GetLogColor(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Trace:   return Colors::GrayLight;
        case LogLevel::Debug:   return Colors::White;
        case LogLevel::Info:    return Colors::Green;
        case LogLevel::Warning: return Colors::Yellow;
        case LogLevel::Error:   return Colors::Red;
        case LogLevel::Fatal:   return Colors::White; // Inverted with Red
    }

    // Should never get here.
    ENSURE(false, "Reached unexpected scope.");
    return Colors::White;
}

std::string Logger::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    // Truncate the time down to centiseconds (2 decimal places)
    auto now_cs = std::chrono::floor<std::chrono::duration<int64_t, std::centi>>(now);
    return std::format("{:%H:%M:%S}", now_cs);
}

void Logger::LogMessage(LogLevel level, const std::source_location& location, const std::string& message)
{
    SCOPE_CPU;

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
    // Format: [HH:MM:SS] [INFO] [File.cpp:42] Log message
    std::string formatted_log = std::format("[{}] {} [{}:{}] {}",
        GetTimestamp(),
        LevelToString(level),
        filename,
        location.line(),
        message
    );

    std::string color_code = GetLogColor(level).ansi();
    const std::string ansi_reset = "\x1b[0m";

    // Use 24-bit True Color for the background (48;2;R;G;B)
    // 1 = Bold, 97 = White Text, 48;2;150;0;0 = Dark Red Background
    if (level == LogLevel::Fatal)
    {
        color_code = "\x1b[1;97;48;2;150;0;0m";
    }

    // Lock the mutex so threads don't scramble standard out or file writes
    std::lock_guard lock(s_LogMutex);

    // Terminal output
    if (level == LogLevel::Fatal || level == LogLevel::Error)
    {
        std::cerr << color_code << formatted_log << ansi_reset << std::endl;
    }
    else
    {
        std::cout << color_code << formatted_log << ansi_reset << std::endl;
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

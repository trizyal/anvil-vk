// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file EngineConfig.h
 * @brief Parsers for loading engine/renderer configurations from .ini files.
 */

#include <fstream>
#include <optional>
#include <string>

#include <glm/glm.hpp>

#include "Console.h"
#include "Logger.h"

/**
 * @details
 * ### Engine Console Variables
 * The following CVars configure the engine.
 * `InitOnly` variables can be set in the `Engine.ini`
 * file or via command-line arguments (e.g., `-w.width 1920`), but cannot be modified
 * at runtime via the developer console.
 * `ReadOnly` variabled have to be set in the source code itself and are compile time variables.
 * For example, the default config that the engine reads.
 *
 * | CVar Name             | Type   | Default       | Flags    |
 * |-----------------------|--------|---------------|----------|
 * | w.title               | String | Application   | InitOnly |
 * | w.width               | Int    | 1280          | InitOnly |
 * | w.height              | Int    | 720           | InitOnly |
 * | sys.attachrenderdoc   | Bool   | false         | InitOnly |
 * | sys.attachtracy       | Bool   | true          | InitOnly |
 * | sys.logdir            | String | saved/logs    | InitOnly |
 * | tracy.viewerpath      | String | Tracy.exe     | None     |
 * | tracy.capturepath     | String | tracy-capture | None     |
 * | e.logverbosity        | Int    | 5             | None     |
 * | r.shadowmapsize       | Int    | 1048          | None     |
 * | r.debugmode           | Int    | 0             | None     |
 * | r.freezerendering     | Bool   | false         | None     |
 * | r.frustumculling      | Bool   | true          | None     |
 */

/**
 * @brief Loads and parsed engine config cvars.
 */
class EngineConfig
{
public:
    /**
     * @brief Parses and '.ini' file and feeds key-value pairs to the CVar console.
     * @param filepath Path to the config file.
     */
    static void LoadAndApply(const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            LOG_ERROR("Could not open engine config file: {}.", filepath);
            LOG_ERROR("Default values will be used.");
            return;
        }

        LOG_INFO("Config file {} parsing.", filepath);
        std::string line;
        while (std::getline(file, line))
        {
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos || line[first] == '#' || line[first] == ';')
            {
                // blank or a commented line: skip
                continue;
            }

            line = line.substr(first,std::string::npos);
            if (line.front() == '[' && line.back() == ']')
            {
                // title line: skip
                continue;
            }

            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos)
            {
                // no '=' in line, hence no value actually set: skip
                continue;
            }

            std::string key = line.substr(0, eq_pos);
            std::string val = line.substr(eq_pos + 1, std::string::npos);

            // clean up the key and val
            key.erase(key.find_last_not_of(" \t\r\n") + 1, std::string::npos);
            val.erase(0, val.find_first_not_of(" \t\r\n"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1, std::string::npos);

            std::string command = std::format("{} {}", key, val);

            LOG_TRACE("Found setting: {}", command);
            Console::Execute(command);
        }
        LOG_INFO("Applied Engine Config: {}", filepath);
    }
};


#if DEPRECATED
struct EngineConfig
{
    /** Absolute or relative path to the source .ini configuration file. */
    std::string configPath;

    std::optional<std::string> windowTitle;
    std::optional<glm::vec2> windowDimensions;

    std::optional<int> vulkanValidationLayers;

    std::optional<int> enableRenderdoc;
    std::optional<std::string> renderdocPath;

    std::optional<int> enableTracy;
    std::optional<std::string> tracyPath;

    std::optional<int> enableShadows;
    std::optional<int> shadowMapSize;

    std::optional<std::string> assetDir;
    std::optional<std::string> shadersDir;
    std::optional<std::string> logsDir;

    std::optional<int> logVerbosity;
};
#endif

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file EngineConfig.h
 * @brief Parsers for loading engine/renderer configurations from .ini files.
 */

#include <optional>
#include <string>

#include <glm/glm.hpp>

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

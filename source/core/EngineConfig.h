// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ENGINECONFIG_H
#define ENGINECONFIG_H

/**
 * @file EngineConfig.h
 * @brief Data structures and parsers for loading engine/renderer configurations from .ini files.
 */

#include <optional>
#include <string>

#include <glm/glm.hpp>


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

#endif // ENGINECONFIG_H

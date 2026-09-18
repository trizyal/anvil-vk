// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SCENECONFIG_H
#define ANVIL_VK_SCENECONFIG_H

/**
 * @file SceneConfig.h
 * @brief Data structures and parsers for loading scene configurations from .ini files.
 */

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <optional>

#include <glm/glm.hpp>

/**
 * @brief Represents a deserialized scene configuration loaded from disk.
 *
 * Maps parameters from a `.ini` file into strongly-typed C++ data, including model paths,
 * initial camera transforms, and global lighting settings. Uses std::optional for values
 * that might be omitted in the configuration file.
 */
struct SceneConfig
{
    /** Absolute or relative path to the source .ini configuration file. */
    std::string configPath;

    std::string sceneName = "Unnamed Scene";

    /** The file path to the primary 3D model (glTF/glb) for this scene. */
    std::string modelPath;

    // Optionals: Only hold a value if explicitly defined in the .ini file
    std::optional<glm::vec3> cameraPosition;
    std::optional<float> cameraSpeed;
    std::optional<float> cameraFovDegrees;

    std::optional<glm::vec4> lightDirection;
    std::optional<glm::vec4> lightColor;
    std::optional<glm::vec4> ambientColor;

    /**
     * @brief Parses a `.ini` configuration file and populates a SceneConfig structure.
     *
     * Iterates through the file line-by-line, stripping whitespace and comments, and maps
     * key-value pairs to the corresponding SceneConfig fields.
     *
     * @param filePath The absolute or relative path to the `.ini` file.
     * @param outConfig Reference to the SceneConfig object to populate.
     * @return True if the file was parsed successfully and a model path was found; false otherwise.
     *
     * @note filePath cannot be a reference as the string it will reference is outConfig.configPath
     * and we reset outConfig and subsequently loose the actual string being referenced by filePath.
     */
    static bool LoadFromFile(const std::string filePath, SceneConfig& outConfig)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            std::cerr << "[SceneConfig] Could not open config file: " << filePath << std::endl;
            return false;
        }

        outConfig = SceneConfig(); // Reset struct
        outConfig.configPath = filePath;

        std::string line;

        auto parse_vec3 = [](const std::string& val)
        {
            std::stringstream ss(val);
            glm::vec3 v(0.0f);
            ss >> v.x >> v.y >> v.z;
            return v;
        };

        auto parse_vec4 = [](const std::string& val)
        {
            std::stringstream ss(val);
            glm::vec4 v(0.0f);
            ss >> v.x >> v.y >> v.z >> v.w;
            return v;
        };

        while (std::getline(file, line))
        {
            size_t first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos || line[first] == '#' || line[first] == ';')
            {
                continue;
            }

            // remove everything before "first"
            line = line.substr(first,std::string::npos);
            if (line.front() == '[' && line.back() == ']')
            {
                continue;
            }

            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos)
            {
                continue;
            }

            std::string key = line.substr(0, eq_pos);
            std::string val = line.substr(eq_pos + 1, std::string::npos);

            // clean up the key and val
            key.erase(key.find_last_not_of(" \t") + 1, std::string::npos);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1, std::string::npos);

            if      (key == "name")             outConfig.sceneName = val;
            else if (key == "model")            outConfig.modelPath = val;
            else if (key == "position")         outConfig.cameraPosition = parse_vec3(val);
            else if (key == "speed")            outConfig.cameraSpeed = std::stof(val);
            else if (key == "fov")              outConfig.cameraFovDegrees = std::stof(val);
            else if (key == "light_direction")  outConfig.lightDirection = parse_vec4(val);
            else if (key == "light_color")      outConfig.lightColor = parse_vec4(val);
            else if (key == "ambient_color")    outConfig.ambientColor = parse_vec4(val);
        }

        return !outConfig.modelPath.empty();
    }
};

#endif //ANVIL_VK_SCENECONFIG_H

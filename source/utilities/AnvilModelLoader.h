// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_MODELLOADER_H
#define ANVIL_VK_MODELLOADER_H

#include <string>
#include <vector>
#include <glm/glm.hpp>

// CPU representation of a vertex
struct MeshVertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 uv;
};

// CPU representation of a 3D model
struct AnvilMesh
{
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::string texturePath;
};

namespace AnvilModelLoader
{
    // Only returns CPU data
    AnvilMesh LoadGLTF(const std::string& filePath);
}

#endif //ANVIL_VK_MODELLOADER_H

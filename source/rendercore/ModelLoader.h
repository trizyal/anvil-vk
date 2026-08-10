// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_MODELLOADER_H
#define ANVIL_VK_MODELLOADER_H

/**
 * @file ModelLoader.h
 * @brief Utilities for loading 3D model files from disk into CPU-side mesh representations.
 */

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

/**
 * @brief CPU-side representation of a single mesh vertex.
 *
 * Interleaved format designed to be directly copied into Vulkan GPU vertex buffers.
 */
struct MeshVertex
{
    glm::vec3 position; /**< 3D position coordinates in local object space. */
    glm::vec3 normal;
    glm::vec2 uv;       /**< 2D texture coordinates for sampling diffuse/albedo maps. */
};

/**
 * @brief CPU-side container for indexed 3D geometry and associated material data.
 */
struct CPUMesh_Single
{
    std::vector<MeshVertex> vertices; /**< Contiguous list of unique vertex attributes. */
    std::vector<uint32_t> indices;    /**< Index list defining triangle faces (3 indices per triangle). */
    std::string texturePath;          /**< Absolute or relative file path to the associated diffuse/albedo texture. */
};

struct CPUTexture
{
    std::string name;
    std::string imagePath;
};

struct CPUMaterial
{
    std::string name;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    int baseColorTextureIndex = -1;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
};

struct CPUMeshPrimitive
{
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    int materialIndex = -1;
};

struct CPUMesh
{
    std::string name;
    std::vector<CPUMeshPrimitive> primitives;
};

struct CPUNode
{
    std::string name;
    int meshIndex = -1;
    int parentIndex = -1;
    std::vector<int> children;

    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 localMatrix = glm::mat4(1.0f);
    glm::mat4 worldMatrix = glm::mat4(1.0f);
};

struct CPUModel
{
    std::vector<CPUTexture> textures;
    std::vector<CPUMaterial> materials;
    std::vector<CPUMesh> meshes;
    std::vector<CPUNode> nodes;
    std::vector<int> sceneRootNodes;
};

/**
 * @brief Free functions for parsing and extracting asset data from disk.
 */
namespace ModelLoader
{
    /**
     * @brief Parses a glTF 2.0 file from disk and extracts its primary mesh and texture data.
     *
     * Reads `.gltf` or `.glb` files, extracting vertex positions, vertex colors, texture
     * coordinates, and triangle indices into standard CPU vectors. This function performs
     * disk I/O and parsing only; it does not allocate any Vulkan GPU resources.
     *
     * @param filePath Path to the `.gltf` or `.glb` file on disk.
     * @return CPUModel populated with extracted glTF scene data.
     *
     * @throws std::runtime_error If the file cannot be read, or if parsing fails.
     */
    CPUModel LoadGLTF(const std::string& filePath);

    /**
     * @brief Legacy convenience loader that returns the first mesh/primitive only.
     *
     * Reads `.gltf` or `.glb` files, extracting vertex positions, vertex colors, texture
     * coordinates, and triangle indices into standard CPU vectors, for one / first mesh in the file.
     *
     * @param filePath Path to the `.gltf` or `.glb` file on disk.
     * @return CPUMesh_Single populated with the extracted vertex, index, and material path data.
     *
     * @throws std::runtime_error If the file cannot be read, or if parsing fails.
     */
    CPUMesh_Single LoadSingleMeshGLTF(const std::string& filePath);
} //AnvilModelLoader

#endif //ANVIL_VK_MODELLOADER_H

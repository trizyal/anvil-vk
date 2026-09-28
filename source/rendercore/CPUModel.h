// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_CPUMODEL_H
#define ANVIL_VK_CPUMODEL_H

/**
 * @file CPUModel.h
 * @brief Utilities for loading 3D model files from disk into CPU-side mesh representations.
 */

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Frustum.h"
#include "Index32.h"

/**
 * @brief CPU-side representation of a single mesh vertex.
 *
 * Interleaved format designed to be directly copied into Vulkan GPU vertex buffers.
 */
struct MeshVertex
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec2 uv = glm::vec2(0.0f);
    glm::vec4 tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

    // Max 4 bones per vertex
    glm::uvec4 joints = glm::uvec4(0);
    glm::vec4 weights = glm::vec4(0.0f);
};

/**
 * @brief CPU-side texture metadata extracted from glTF.
 */
struct CPUTexture
{
    std::string name;
    std::string imagePath;
    bool isSRGB = true; // false = UNORM
};

/**
 * @brief CPU-side material metadata extracted from glTF.
 *
 * baseColorTextureIndex indexes CPUModel::textures.
 */
struct CPUMaterial
{
    std::string name;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);

    Index32 baseColorTextureIndex = -1;
    Index32 normalTextureIndex = -1;
    Index32 metallicRoughnessTextureIndex = -1;

    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float alphaCutoff = 0.5f;
};

/**
 * @brief CPU-side draw primitive.
 *
 * @note materialIndex indexes CPUModel::materials.
 */
struct CPUMeshPrimitive
{
    std::vector<MeshVertex> vertices;
    std::vector<Index32> indices;
    Index32 materialIndex = -1;

    /** Local-space bounding box calculated from this primitive's vertex positions. */
    AABB localBounds;
};

/**
 * @brief CPU-side mesh containing one or more primitives.
 */
struct CPUMesh
{
    std::string name;
    std::vector<CPUMeshPrimitive> primitives;
};

/**
 * @brief CPU-side skin.
 *
 * @note skeletonRootNode indexes CPUModel::nodes.
 */
struct CPUSkin
{
    std::string name;
    Index32 skeletonRootNode = -1;
    std::vector<Index32> jointNodes; /**< CPU nodes that act as bones */
    std::vector<glm::mat4> inverseBindMatrices; /**< Rest pose inverse matrices. */
};

/**
 * @brief CPU-side scene node.
 *
 * @note meshIndex indexes CPUModel::meshes.
 * @note skinIndex indexes CPUModel::skins.
 * @note parentIndex indexes CPUModel::nodes.
 */
struct CPUNode
{
    std::string name;
    Index32 meshIndex = -1;
    Index32 skinIndex = -1;
    Index32 parentIndex = -1;
    std::vector<Index32> children;

    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 localMatrix = glm::mat4(1.0f);
    glm::mat4 worldMatrix = glm::mat4(1.0f);
};

/**
 * @brief
 */
enum class AnimationPath
{
    Unknown,
    Translation,
    Rotation,
    Scale
};

/**
 * @brief A single animation channel targeting a node's transform.
 */
struct CPUAnimationChannel
{
    Index32 targetNodeIndex = -1;
    AnimationPath path = AnimationPath::Unknown;

    std::vector<float> keyframeTimes;
    std::vector<glm::quat> keyframeRotations;
    std::vector<glm::vec3> keyframeTranslations;
    std::vector<glm::vec3> keyframeScales;
};

/**
 * @brief A full animation clip containing multiple channels.
 */
struct CPUAnimation
{
    std::string name;
    float duration = 0.0f;
    std::vector<CPUAnimationChannel> channels;
};

/**
 * @brief Full CPU-side model and scene data.
 *
 * @note Currently this class is copyable and movable both.
 */
class CPUModel
{
public:
    std::vector<CPUTexture> textures;
    std::vector<CPUMaterial> materials;
    std::vector<CPUMesh> meshes;
    std::vector<CPUNode> nodes;
    std::vector<Index32> sceneRootNodes;

    std::vector<CPUAnimation> animations;
    std::vector<CPUSkin> skins;

    /**
     * @brief Parses a glTF 2.0 file from disk and populates CPUModel.
     *
     * Reads `.gltf` or `.glb` files, extracting vertex positions, vertex colors, texture
     * coordinates, and triangle indices into standard CPU vectors. This function performs
     * disk I/O and parsing only; it does not allocate any Vulkan GPU resources.
     *
     * @param filePath Path to the `.gltf` or `.glb` file on disk.
     *
     * @throws std::runtime_error If the file cannot be read, or if parsing fails.
     */
    void loadGLTF(const std::string& filePath);

    /**
     * @brief Update all matrices in nodes according to their parents
     */
    void updateAllMatrices();

    /**
     * @brief Interpolates and applies animation keyframes to the model's nodes.
     *
     * @param animationIndex The index of the CPUModel::animation to play.
     * @param time The current playback time in seconds.
     */
    void applyAnimation(Index32 animationIndex, float time);

    /**
     * @brief Computes the final skinning matrices for all joints affecting a specific skinned mesh node.
     *
     * Calculates the transformation matrix for each joint by multiplying the inverse of the
     * mesh's world matrix, the joint's current animated world matrix, and the joint's inverse
     * bind matrix. The resulting matrices represent the delta transform from the bind pose
     * and are intended to be uploaded to a GPU Storage Buffer (SSBO) for vertex skinning.
     *
     * @param nodeIndex The index of the CPUNode representing the rigged mesh. If the node
     * does not have an associated skin, the output vector is cleared.
     *
     * @param matrices A reference to a vector that will be resized and populated with
     * the computed glm::mat4 joint matrices.
     */
    void computeJointMatrices(Index32 nodeIndex, std::vector<glm::mat4>& matrices) const;
};

#endif //ANVIL_VK_CPUMODEL_H

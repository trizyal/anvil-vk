// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_MODELLOADER_H
#define ANVIL_VK_MODELLOADER_H

/**
 * @file CPUModel.h
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
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec2 uv= glm::vec2(0.0f);

    // Max 4 bones per vertex
    glm::uvec4 joints = glm::uvec4(0);
    glm::vec4 weights = glm::vec4(0.0f);
};

/**
 * @brief Legacy CPU-side container for indexed 3D geometry and associated material data.
 */
struct CPUMesh_Single
{
    std::vector<MeshVertex> vertices; /**< Contiguous list of unique vertex attributes. */
    std::vector<uint32_t> indices;    /**< Index list defining triangle faces (3 indices per triangle). */
    std::string texturePath;          /**< Absolute or relative file path to the associated diffuse/albedo texture. */
};

/**
 * @brief CPU-side texture metadata extracted from glTF.
 */
struct CPUTexture
{
    std::string name;
    std::string imagePath;
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

    int baseColorTextureIndex = -1;
    int normalTextureIndex = -1;
    int metallicRoughnessTextureIndex = -1;

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
    std::vector<uint32_t> indices;
    int materialIndex = -1;
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
    int skeletonRootNode = -1;
    std::vector<int> jointNodes; /**< CPU nodes that act as bones */
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
    int meshIndex = -1;
    int skinIndex = -1;
    int parentIndex = -1;
    std::vector<int> children;

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
    int targetNodeIndex = -1;
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
    std::vector<int> sceneRootNodes;

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
    void applyAnimation(int animationIndex, float time);

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
    void computeJointMatrices(int nodeIndex, std::vector<glm::mat4>& matrices) const;
};

/**
 * @brief Free functions for parsing and extracting asset data from disk.
 */
namespace ModelLoader
{
    /**
     * @brief Legacy function to parse a glTF 2.0 file from disk and extracts its primary mesh and texture data.
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
    [[deprecated("CPUModel now provides loadGLTF() as a member function.")]]
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
    [[deprecated("CPUModel now provides loadGLTF() as a member function.")]]
    CPUMesh_Single LoadSingleMeshGLTF(const std::string& filePath);

    /**
     * @brief Legacy convenience function to update all matrices in nodes according to their parents.
     *
     * @param cpuModel CPUModel that stores the model data and matrices.
     *
     * @note Legacy code. No improvements will be made here.
     * @see CPUModel::updateAllMatrices().
     */
    [[deprecated("CPUModel now provides updateAllMatrices() as a member function.")]]
    void UpdateAllMatrices(CPUModel& cpuModel);
} //AnvilModelLoader

#endif //ANVIL_VK_MODELLOADER_H

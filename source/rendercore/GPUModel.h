// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_GPUMODEL_H
#define ANVIL_VK_GPUMODEL_H

/**
 * @file GPUModel.h
 * @brief Converts CPUModel data into GPU textures, mesh buffers, material bindings, and draw items.
 */

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <volk.h>

#include "AnvilMaterial.h"
#include "GPUMesh.h"
#include "MaterialInstance.h"
#include "CPUModel.h"
#include "GPUTexture.h"

class VulkanContext;

constexpr uint8_t WhiteColor[4] = {255, 255, 255, 255};
constexpr uint8_t NormalColor[4] = {128, 128, 255, 255}; // Flat Z-up normal
constexpr uint8_t TransparentColor[4] = {0, 0, 0, 0}; //

/**
 * @brief Max number of Joints allowed.
 * 256 glm::mat4 matrices (16KB total)
 */
constexpr size_t MAX_BONES = 256;

/**
 * @brief Per-material GPU-side binding packet.
 *
 * materialIndex maps back to CPUModel::materials.
 */
struct GPUModelMaterial
{
    int materialIndex = -1; /**< Index corresponding to CPUModel::materials. */
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    MaterialInstance instance; /**< Descriptor set manager for this specific material. */
};

/**
 * @brief One renderable primitive instance in the flattened draw list.
 *
 * Represents a single Vulkan draw call containing exactly what is needed to render it.
 *
 * gpuMeshIndex indexes GPUModel::gpuMeshes.
 * gpuMaterialIndex indexes GPUModel::gpuMaterials.
 */
struct GPUModelDrawItem
{
    uint32_t gpuMeshIndex = 0; /**< Index into GPUModel::gpuMeshes. */
    int gpuMaterialIndex = -1; /**< Index into GPUModel::gpuMaterials. */
    glm::mat4 worldMatrix = glm::mat4(1.0f);
    int cpuNodeIndex = -1; /**< Map back to CPU node for animation matrix updates. */

    /** Local-space bounding box used for fast CPU-side frustum culling tests. */
    AABB localBounds;
};

/**
 * @brief GPU-side representation of a CPUModel.
 *
 * Owns uploaded textures, uploaded primitive mesh buffers, per-material
 * descriptor sets, and draw items.
 *
 * @note This class in non-copyable. Moving is allowed.
 */
class GPUModel
{
public:
    GPUModel() = default;
    ~GPUModel() = default;

    GPUModel(const GPUModel&) = delete;
    GPUModel& operator=(const GPUModel&) = delete;

    GPUModel(GPUModel&&) noexcept;
    GPUModel& operator=(GPUModel&&) noexcept;

private:
    VulkanContext* pContext = nullptr;

public:
    GPUTexture defaultWhiteTexture;
    GPUTexture defaultNormalTexture;
    GPUTexture defaultTransparentTexture;

    MaterialInstance modelSet; // Set 1

    std::vector<GPUTexture> textures;
    std::vector<GPUMesh> gpuMeshes;
    std::vector<GPUModelMaterial> gpuMaterials;
    std::vector<GPUModelDrawItem> drawItems;

    GPUBuffer jointBuffer;

    /** SSBO for model matrices. */
    GPUBuffer modelMatricesBuffer;

    /**
     * @brief Legacy function to upload a CPUModel to GPU-side resources and generates a draw list.
     */
    [[deprecated("Use the multi-set architecture instead.")]]
    void createGPUModel(
        VulkanContext& inContext,
        const CPUModel& inModel,
        const AnvilMaterial& inMaterial,
        const std::string& sceneBufferName,
        const GPUBuffer& sceneBuffer,
        const std::string& textureName
    );

    /**
     * @brief Uploads a CPUModel to GPU-side resources and generates a draw list.
     *
     * @param inContext Reference to the active Anvil Vulkan context.
     * @param inModel Reference to the model structure on CPU.
     * @param inMaterial
     */
    void createGPUModel(
        VulkanContext& inContext,
        const CPUModel& inModel,
        const AnvilMaterial& inMaterial
    );

    /**
     * @brief Safely destroys all GPU-side resources managed by this model.
     *
     * Releases VMA allocations, destroys Vulkan buffers (joints, model matrices),
     * destroys textures, and clears all draw items and materials.
     */
    void destroyGPUModel();

    /**
     * @brief Synchronizes the GPU draw list matrices with the latest CPU node matrices.
     *
     * Iterates through the draw items, extracts the updated world matrices from the
     * associated CPU nodes, and uploads them to the model matrices SSBO.
     *
     * @param inModel Reference to the CPU model containing the updated node transforms.
     */
    void updateTransforms(const CPUModel& inModel);

    /**
     * @brief Computes and uploads the latest skeletal joint matrices to the GPU for skinning.
     *
     * Calculates the absolute joint transforms relative to the bind pose and writes
     * the data to the joint SSBO.
     *
     * @param inModel Reference to the CPU model containing the animated skeleton data.
     */
    void updateJoints(const CPUModel& inModel) const;

private:
    /**
     * @brief Iterates over the CPU model and allocates GPU textures for every material.
     * @param inModel The CPU model providing texture paths and colors.
     */
    void createTextures(const CPUModel& inModel);

    [[deprecated("Use the multi-set architecture instead.")]]
    void createMaterialDescriptorSets(
        const CPUModel& inModel,
        const AnvilMaterial& inMaterial,
        const std::string& sceneBufferName,
        const GPUBuffer& sceneBuffer,
        const std::string& textureName
    );

    /**
     * @brief Allocates and writes Vulkan descriptor sets for the model's global data (Set 1) and materials (Set 2).
     * @param inModel The CPU model containing material metadata.
     * @param inMaterial The AnvilMaterial factory to allocate sets from.
     */
    void createMaterialDescriptorSets(
        const CPUModel& inModel,
        const AnvilMaterial& inMaterial
    );

    /**
     * @brief Flattens CPU nodes and meshes into a linear list of renderable GPU draw items.
     * @param inCPUModel The structured scene graph and mesh data.
     */
    void createMeshesAndDrawItems(const CPUModel& inCPUModel);

    /**
     * @brief Allocates an SSBO capable of holding up to MAX_BONES joint matrices.
     */
    void createJointBuffer();

    /**
     * @brief Allocates a host-visible SSBO to store the latest world transforms for all draw items.
     */
    void createModelMatricesBuffer();
};

#endif //ANVIL_VK_GPUMODEL_H

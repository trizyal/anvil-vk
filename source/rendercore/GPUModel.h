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
#include "TextureLoader.h"

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
    AnvilTexture defaultWhiteTexture;
    AnvilTexture defaultNormalTexture;
    AnvilTexture defaultTransparentTexture;

    MaterialInstance modelSet; // Set 1

    std::vector<AnvilTexture> textures;
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
     * @brief Synchronizes the GPU draw list matrices with the latest CPU node matrices.
     */
    void updateTransforms(const CPUModel& inModel);

    void updateJoints(const CPUModel& inModel) const;

    void destroyGPUModel();

private:
    void createTextures(const CPUModel& inModel);

    [[deprecated("Use the multi-set architecture instead.")]]
    void createMaterialDescriptorSets(
        const CPUModel& inModel,
        const AnvilMaterial& inMaterial,
        const std::string& sceneBufferName,
        const GPUBuffer& sceneBuffer,
        const std::string& textureName
    );

    void createMaterialDescriptorSets(
        const CPUModel& inModel,
        const AnvilMaterial& inMaterial
    );

    void createMeshesAndDrawItems(const CPUModel& inCPUModel);

    void createJointBuffer();
    void createModelMatricesBuffer();
};

#endif //ANVIL_VK_GPUMODEL_H

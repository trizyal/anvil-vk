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
#include "ModelLoader.h"
#include "TextureLoader.h"

class VulkanContext;

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
    std::vector<AnvilTexture> textures;
    std::vector<GPUMesh> gpuMeshes;
    std::vector<GPUModelMaterial> gpuMaterials;
    std::vector<GPUModelDrawItem> drawItems;

    void createGPUModel();

    void destroyGPUModel();
};

#endif //ANVIL_VK_GPUMODEL_H

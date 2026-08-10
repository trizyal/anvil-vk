// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_GPUMESH_H
#define ANVIL_VK_GPUMESH_H

/**
 * @file GPUMesh.h
 * @brief Container for GPU-side vertex and index buffers representing a renderable 3D mesh.
 */

#include <array>
#include <volk.h>
#include <glm/glm.hpp>

#include "GPUBuffer.h"
#include "VulkanContext.h"
#include "ModelLoader.h"

/**
 * @brief Encapsulates GPU vertex and index buffers for an indexed 3D mesh.
 *
 * Manages the upload of CPU-side AnvilMesh geometry data into device-local GPU buffers
 * and provides static reflection helpers to configure Vulkan pipeline vertex input states.
 *
 * @note This class in non-copyable. Moving is allowed.
 */
class GPUMesh
{
public:
    GPUMesh() = default;
    ~GPUMesh() = default;

    GPUMesh(const GPUMesh&) = delete;
    GPUMesh& operator=(const GPUMesh&) = delete;

    GPUMesh(GPUMesh&&) noexcept = default;
    GPUMesh& operator=(GPUMesh&&) noexcept = default;

    /** GPU buffer containing interleaved MeshVertex attributes. */
    GPUBuffer vertexBuffer;

    /** GPU buffer containing 32-bit triangle indices. */
    GPUBuffer indexBuffer;

    /** Total number of indices to draw. */
    uint32_t indexCount = 0;

private:
    /** Cached context used for self-contained destruction. */
    const VulkanContext* ptrAContext = nullptr;

public:
    /**
     * @brief Allocates GPU buffers and uploads CPU-side geometry data from an AnvilMesh.
     *
     * @param inContext Reference to the active Anvil Vulkan context.
     * @param inMesh CPU-side mesh containing vertex and index vectors to upload.
     *
     * @see CPUMesh_Single
     */
    void createGPUMesh(const VulkanContext& inContext, const CPUMesh_Single& inMesh);

    /**
     * @brief Allocates GPU buffers and uploads CPU-side geometry data from an MeshPrimitive.
     *
     * @param inContext Reference to the active Anvil Vulkan context.
     * @param inMeshPrimitive CPU-side mesh primitive containing vertex and index vectors to upload.
     *
     * @see CPUMeshPrimitive
     * @see CPUMesh
     */
    void createGPUMesh(const VulkanContext& inContext, const CPUMeshPrimitive& inMeshPrimitive);

    /**
     * @brief Destroys the underlying GPU vertex and index buffers.
     */
    void destroyGPUMesh();

    /**
     * @brief Returns the Vulkan vertex input binding description for interleaved MeshVertex data.
     *
     * Configures binding slot 0 to consume per-vertex data at a stride of sizeof(MeshVertex).
     *
     * @return A populated VkVertexInputBindingDescription structure for pipeline creation.
     */
    static VkVertexInputBindingDescription getBindingDescription();

    /**
     * @brief Returns attribute descriptions mapping MeshVertex fields to shader locations.
     *
     * Configures three vertex shader input attributes:
     * * Location 0: 3D Position (`glm::vec3`, `VK_FORMAT_R32G32B32_SFLOAT`)
     * * Location 1: RGB Color (`glm::vec3`, `VK_FORMAT_R32G32B32_SFLOAT`)
     * * Location 2: 2D UV Coordinates (`glm::vec2`, `VK_FORMAT_R32G32_SFLOAT`)
     *
     * @return A 3-element array of VkVertexInputAttributeDescription structures.
     */
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

#endif //ANVIL_VK_GPUMESH_H

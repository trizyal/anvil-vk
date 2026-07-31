// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_MESHBUFFER_H
#define ANVIL_VK_MESHBUFFER_H

/**
 * @file AnvilMeshBuffer.h
 * @brief Container for GPU-side vertex and index buffers representing a renderable 3D mesh.
 */

#include <array>
#include <volk.h>
#include <glm/glm.hpp>

#include "AnvilBuffer.h"
#include "AnvilVulkanContext.h"
#include "AnvilModelLoader.h"

/**
 * @brief Encapsulates GPU vertex and index buffers for an indexed 3D mesh.
 *
 * Manages the upload of CPU-side AnvilMesh geometry data into device-local GPU buffers
 * and provides static reflection helpers to configure Vulkan pipeline vertex input states.
 *
 * @note This class in non-copyable. Moving is allowed.
 */
class AnvilMeshBuffer
{
public:
    AnvilMeshBuffer() = default;
    ~AnvilMeshBuffer() = default;

    AnvilMeshBuffer(const AnvilMeshBuffer&) = delete;
    AnvilMeshBuffer& operator=(const AnvilMeshBuffer&) = delete;

    AnvilMeshBuffer(AnvilMeshBuffer&&) noexcept = default;
    AnvilMeshBuffer& operator=(AnvilMeshBuffer&&) noexcept = default;

    /** GPU buffer containing interleaved MeshVertex attributes. */
    AnvilBuffer vertexBuffer;

    /** GPU buffer containing 32-bit triangle indices. */
    AnvilBuffer indexBuffer;

    /** Total number of indices to draw. */
    uint32_t indexCount = 0;

private:
    /** Cached context used for self-contained destruction. */
    const AnvilVulkanContext* ptrAContext = nullptr;

public:
    /**
     * @brief Allocates GPU buffers and uploads CPU-side geometry data from an AnvilMesh.
     *
     * @param inContext Reference to the active Anvil Vulkan context.
     * @param inMesh CPU-side mesh containing vertex and index vectors to upload.
     *
     * @see AnvilMesh
     */
    void createAnvilMeshBuffer(const AnvilVulkanContext& inContext, const AnvilMesh& inMesh);

    /**
     * @brief Destroys the underlying GPU vertex and index buffers.
     */
    void destroyAnvilMeshBuffer();

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

#endif //ANVIL_VK_MESHBUFFER_H

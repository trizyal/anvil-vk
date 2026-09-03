// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_GBUFFER_H
#define ANVIL_VK_GBUFFER_H

/**
 * @file GBuffer.h
 * @brief Geometry Buffer (G-Buffer) container for deferred rendering attachments.
 */

#include <volk.h>
#include "GPUTexture.h"

class VulkanContext;

/**
 * @brief Manages offscreen GPU textures required for deferred rendering passes.
 *
 * Encapsulates render target attachments (Albedo, World Normals, PBR parameters,
 * World Positions, and Depth) populated during the geometry pass to defer lighting calculations.
 */
class GBuffer
{
public:
    GBuffer() = default;
    ~GBuffer() = default;

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;

    GBuffer(GBuffer&&) noexcept = default;
    GBuffer& operator=(GBuffer&&) noexcept = default;

    /** RGB: Base color / Albedo, Alpha: Unused. Format: R8G8B8A8_UNORM */
    GPUTexture albedo;

    /** RGB: World-space normal vector, Alpha: Unused. Format: R16G16B16A16_SFLOAT */
    GPUTexture normal;

    /** R: Unused, G: Roughness, B: Metallic, Alpha: Unused. Format: R8G8B8A8_UNORM */
    GPUTexture pbr;

    /** RGB: World-space position vector, Alpha: Unused. Format: R16G16B16A16_SFLOAT */
    GPUTexture worldPosition;

    /** Depth attachment for depth testing and position reconstruction. Format: D32_SFLOAT */
    GPUTexture depth;

    /** Current pixel resolution (width and height) of all G-Buffer attachments. */
    VkExtent2D currentExtent = {0, 0};

    /**
     * @brief Allocates or resizes all GPU attachment textures matching the target extent.
     *
     * @param inContext Active Vulkan context providing device handles and memory allocators.
     * @param extent Target pixel resolution for the offscreen attachments.
     */
    void create(const VulkanContext& inContext, VkExtent2D extent);

    /**
     * @brief Releases GPU memory and destroys handles for all underlying attachment textures.
     */
    void destroy();
};


#endif //ANVIL_VK_GBUFFER_H

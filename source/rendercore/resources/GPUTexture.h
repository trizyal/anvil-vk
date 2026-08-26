// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_GPUTEXTURE_H
#define ANVIL_VK_GPUTEXTURE_H

/**
 * @file GPUTexture.h
 * @brief Move-only wrapper around Vulkan Image, ImageView and Vulkan Memory Allocator (VMA) allocations.
 */

#include <string>
#include <volk.h>
#include <vk_mem_alloc.h>

#include "VulkanContext.h"
#include "DebugNames.h"

/**
 * @brief Manages the lifecycle of a Vulkan Image, ImageView, Sampler and its backing VMA memory allocation.
 *
 * Implements move-only semantics to safely transfer buffer ownership across scopes
 * and STL containers without risking accidental double-free memory corruption.
 *
 * @note Because GPU memory destruction often requires synchronization with the render loop,
 * the destructor does not automatically free GPU memory. You must call destroyTexture() explicitly.
 *
 * @warning Each texture does not need a unique sampler, so need to remove sampler from this block.
 *
 * @note Copying this class is disallowed. Moving is allowed.
 */
class GPUTexture
{
public:
    GPUTexture() = default;
    ~GPUTexture() = default;

    GPUTexture(const GPUTexture&) = delete;
    GPUTexture& operator=(const GPUTexture&) = delete;

    GPUTexture(GPUTexture&& other) noexcept;
    GPUTexture& operator=(GPUTexture&& other) noexcept;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

private:
    /** Cached context used for self-contained destruction. */
    const VulkanContext* pContext = nullptr;

public:
    /**
     * @brief Loads an image file from disk, uploads it to device-local GPU memory, and generates mipmaps.
     *
     * Reads standard image formats (PNG, JPEG, TGA, etc.) from disk, stages the pixel data,
     * submits a transfer command to upload it to a device-local Vulkan image, and creates an
     * associated image view and default sampler.
     *
     * Base Color (Albedo) should be sRGB because it represents visual colors.
     * Normal, Metallic, and Roughness Maps represent raw math data, not colors. They MUST be linear (UNORM).
     *
     * @param inContext Core Vulkan context used for staging command submission and VMA allocation.
     * @param filepath  Absolute or relative filesystem path to the source image file.
     * @param bIsSRGB true means we load with sRGB, otherwise UNORM.
     *
     * @throws std::runtime_error If file loading fails, or if buffer/image creation commands fail.
     */
    void createTexture(const VulkanContext& inContext, const std::string& filepath, bool bIsSRGB = true);

    /**
     * @brief Releases all GPU resources associated with this texture.
     *
     * Safely checks for non-null handles before destroying the sampler, image view,
     * and freeing the VMA image allocation.
     */
    void destroyTexture();

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format D_DECL());
    void createImageView(uint32_t mipLevels, VkFormat format D_DECL());
    void createSampler(uint32_t mipLevels D_DECL());
};


#endif //ANVIL_VK_GPUTEXTURE_H

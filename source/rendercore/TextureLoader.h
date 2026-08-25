// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_TEXTURELOADER_H
#define ANVIL_VK_TEXTURELOADER_H

/**
 * @file TextureLoader.h
 * @brief Utility structures and functions for loading image assets into device-local GPU textures.
 * @note May need to be turned into a similar class as AnvilBuffer. Right now it mirrors AnvilModelLoader,
 * may need separation there to have texture behave more like a wrapper, like buffer and have a separate Image Class.
 */

#include <string>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "VulkanContext.h"

/**
 * @brief GPU-side container representing a sampled 2D texture.
 *
 * Bundles the Vulkan image, its memory allocation (VMA), the image view, and the texture sampler.
 *
 * @note May need to change in to a class.
 *
 * @warning Each texture does not need a unique sampler, so need to remove sampler from this block.
 */
struct AnvilTexture
{
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    /**
     * @brief Releases all GPU resources associated with this texture.
     *
     * Safely checks for non-null handles before destroying the sampler, image view,
     * and freeing the VMA image allocation.
     *
     * @param inContext Pointer to the root Vulkan context providing the logical device and VMA allocator.
     */
    void destroyAnvilTexture(const VulkanContext* inContext) const
    {
        if (sampler)
        {
            vkDestroySampler(inContext->device, sampler, nullptr);
        }
        if (imageView)
        {
            vkDestroyImageView(inContext->device, imageView, nullptr);
        }
        if (image)
        {
            vmaDestroyImage(inContext->allocator, image, allocation);
        }
    }
};

/**
 * @brief Functions for loading, decoding, and uploading texture files to the GPU.
 */
namespace TextureLoader
{
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
     * @param filepath  Absolute or relative filesystem path to the source image file.
     * @param inContext Core Vulkan context used for staging command submission and VMA allocation.
     * @param bIsSRGB true means we load with sRGB, otherwise UNORM.
     * @return A fully populated AnvilTexture ready for descriptor set binding.
     *
     * @throws std::runtime_error If file loading fails, or if buffer/image creation commands fail.
     */
    AnvilTexture LoadTexture(const std::string& filepath, VulkanContext& inContext, bool bIsSRGB = true);

    /**
     * @brief Create a solid texture image, upload it to device-local GPU memory.
     *
     * @param color The rgba value in unsigned 8-bit format.
     * @param inContext Core Vulkan context used for staging command submission and VMA allocation.
     * @param bIsSRGB true means we create with sRGB, otherwise UNORM.
     * @return A fully populated AnvilTexture ready for descriptor set binding.
     *
     * @throws std::runtime_error If texture creation fails, or if buffer/image creation commands fail.
     */
    AnvilTexture CreateSolidColorTexture(const uint8_t color[4], VulkanContext& inContext);
}

#endif //ANVIL_VK_TEXTURELOADER_H

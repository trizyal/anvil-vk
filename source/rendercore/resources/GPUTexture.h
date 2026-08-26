// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_GPUTEXTURE_H
#define ANVIL_VK_GPUTEXTURE_H

/**
 * @file GPUTexture.h
 * @brief Move-only wrapper around Vulkan Image, ImageView and Vulkan Memory Allocator (VMA) allocations.
 */

#include <volk.h>
#include <vk_mem_alloc.h>

#include "VulkanContext.h"

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

    // GPUTexture(const GPUTexture&) = delete;
    // GPUTexture& operator=(const GPUTexture&) = delete;
    //
    // GPUTexture(GPUTexture&& other) noexcept;
    // GPUTexture& operator=(GPUTexture&& other) noexcept;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    /** Cached context used for self-contained destruction. */
    const VulkanContext* pContext = nullptr;

    /**
     * @brief Releases all GPU resources associated with this texture.
     *
     * Safely checks for non-null handles before destroying the sampler, image view,
     * and freeing the VMA image allocation.
     *
     * @param inContext Pointer to the root Vulkan context providing the logical device and VMA allocator.
     */
    void destroyTexture(const VulkanContext* inContext) const
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


#endif //ANVIL_VK_GPUTEXTURE_H

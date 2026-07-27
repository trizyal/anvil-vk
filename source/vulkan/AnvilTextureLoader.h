// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_TEXTURELOADER_H
#define ANVIL_VK_TEXTURELOADER_H

#include <string>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "AnvilVulkanContext.h"

struct AnvilTexture
{
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    void destroyAnvilTexture(const AnvilVulkanContext* inContext) const
    {
        if (sampler) vkDestroySampler(inContext->anvilDevice, sampler, nullptr);
        if (imageView) vkDestroyImageView(inContext->anvilDevice, imageView, nullptr);
        if (image) vmaDestroyImage(inContext->anvilAllocator, image, allocation);
    }
};

namespace AnvilTextureLoader
{
    AnvilTexture LoadTexture(const std::string& filepath, AnvilVulkanContext& inContext);
}

#endif //ANVIL_VK_TEXTURELOADER_H

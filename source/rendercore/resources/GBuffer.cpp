// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "GBuffer.h"

void GBuffer::create(const VulkanContext& inContext, VkExtent2D extent)
{
    destroy();
    currentExtent = extent;

    const VkImageUsageFlags colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    const VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    albedo.createAttachment(inContext, extent.width, extent.height, VK_FORMAT_R8G8B8A8_UNORM, colorUsage DNAME("GBuffer_Albedo"));
    normal.createAttachment(inContext, extent.width, extent.height, VK_FORMAT_R16G16B16A16_SFLOAT, colorUsage DNAME("GBuffer_Normal"));
    pbr.createAttachment(inContext, extent.width, extent.height, VK_FORMAT_R8G8B8A8_UNORM, colorUsage DNAME("GBuffer_PBR"));
    worldPosition.createAttachment(inContext, extent.width, extent.height, VK_FORMAT_R16G16B16A16_SFLOAT, colorUsage DNAME("GBuffer_WorldPos"));
    depth.createAttachment(inContext, extent.width, extent.height, VK_FORMAT_D32_SFLOAT, depthUsage DNAME("GBuffer_Depth"));
}

void GBuffer::destroy()
{
    albedo.destroyTexture();
    normal.destroyTexture();
    pbr.destroyTexture();
    worldPosition.destroyTexture();
    depth.destroyTexture();
}

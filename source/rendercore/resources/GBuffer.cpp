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

std::vector<VkRenderingAttachmentInfo> GBuffer::getRenderingAttachments()
{
    std::vector<VkRenderingAttachmentInfo> colors;
        colors.push_back(getAttachmentInfo(albedo));
        colors.push_back(getAttachmentInfo(normal));
        colors.push_back(getAttachmentInfo(pbr));
        colors.push_back(getAttachmentInfo(worldPosition));

    return colors;
}

VkRenderingAttachmentInfo GBuffer::getAttachmentInfo(const GPUTexture& texture)
{
    VkRenderingAttachmentInfo attachment_info{};
    attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachment_info.imageView = texture.imageView;
    attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_info.clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

    return attachment_info;
}

VkRenderingAttachmentInfo GBuffer::getDepthAttachmentInfo()
{
    VkRenderingAttachmentInfo attachment_info{};
    attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachment_info.imageView = depth.imageView;
    attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_info.clearValue = {1.0f, 0};

    return attachment_info;
}


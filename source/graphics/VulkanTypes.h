// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file VulkanTypes.h
 */

#include <volk.h>

/**
 * @brief Macro to create CustomType casting functions.
 */
#define DEF_VK_CAST(CustomType, VulkanType) \
    constexpr VulkanType vk(CustomType e) { \
        return static_cast<VulkanType>(e); \
    }


/**
 * @brief Concise wrapper for commonly used Vulkan load operations.
 */
enum class LoadOp : uint32_t
{
    Clear    = VK_ATTACHMENT_LOAD_OP_CLEAR,
    Load     = VK_ATTACHMENT_LOAD_OP_LOAD,
    DontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE
};

/**
 * @brief Concise wrapper for commonly used Vulkan image layouts.
 */
enum class ImageLayout : uint32_t
{
    Undefined       = VK_IMAGE_LAYOUT_UNDEFINED,
    ColorAttachment = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    DepthAttachment = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    ShaderReadOnly  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    Present         = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    TransferSrc     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    TransferDst     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
};

/**
 * @brief Concise wrapper for commonly used Vulkan image and buffer formats.
 */
enum class Format : uint32_t
{
    Undefined           = VK_FORMAT_UNDEFINED,

    // --(Standard Color / Textures)--
    // 8-bit 4-channels unsigned normalised integers (0-255)
    RGBA8_UNORM      = VK_FORMAT_R8G8B8A8_UNORM,

    // 8-bit 4-channels unsigned integers (0-255)
    RGBA8_SRGB       = VK_FORMAT_R8G8B8A8_SRGB,

    // (HDR / G-Buffer Normals & World Pos)
    // 16-bit 4-channels signed floats
    RGBA16_SFLOAT       = VK_FORMAT_R16G16B16A16_SFLOAT,

    // --(Vertex attributes 32-bits)--
    // 32-bit 2-channels signed floats
    RG32_SFLOAT       = VK_FORMAT_R32G32_SFLOAT,

    // 32-bit 3-channels signed floats
    RGB32_SFLOAT    = VK_FORMAT_R32G32B32_SFLOAT,

    // 32-bit 4-channels signed floats
    RGBA32_SFLOAT = VK_FORMAT_R32G32B32A32_SFLOAT,

    // 32-bit 4-channels unsigned integers
    RGBA32_UINT   = VK_FORMAT_R32G32B32A32_UINT,

    // --(Depth Stencil)--
    // 32-bit 1-channel signed float
    D32_SFLOAT          = VK_FORMAT_D32_SFLOAT
};

DEF_VK_CAST(LoadOp, VkAttachmentLoadOp);
DEF_VK_CAST(ImageLayout, VkImageLayout);
DEF_VK_CAST(Format, VkFormat);

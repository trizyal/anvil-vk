// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <string>

#include <volk.h>
#include <vulkan/vk_enum_string_helper.h>

#include "VulkanTypes.h"

/**
 * @brief Macro to create overloaded functions vk_str to convert Vulkan Enums to strings.
 *
 * @see https://github.com/KhronosGroup/Vulkan-Utility-Libraries/blob/main/include/vulkan/vk_enum_string_helper.h
 */
#define DEF_VK_STRING(VulkanType) \
    inline std::string vk_str(VulkanType t) { \
        return string_##VulkanType(t); \
    }

DEF_VK_STRING(VkResult);
DEF_VK_STRING(VkImageLayout);
DEF_VK_STRING(VkFormat);

/**
 * @brief Converts a LoadOp enums into a human-readable string literal.
 *
 * @see LoadOp
 *
 * @param op The LoadOp value.
 * @return A string matching the LoadOp enum name (e.g., "Load").
 */
inline std::string vk_str(const LoadOp op)
{
    switch (op)
    {
#       define CASE_(x) case LoadOp::x: return #x
        CASE_(Clear);
        CASE_(Load);
        CASE_(DontCare);
#       undef CASE_
    }

    // Handle other values gracefully.
    ENSURE(false, "LoadOp Enum not handled");
    std::ostringstream oss;
    oss << "LoadOp(" << static_cast<std::underlying_type_t<LoadOp>>(op) << ")";
    return oss.str();
}

/**
 * @brief Converts a ImageLayout enums into a human-readable string literal.
 *
 * @see ImageLayout
 *
 * @param layout The ImageLayout value.
 * @return A string matching the ImageLayout enum name (e.g., "ColorAttachment").
 */
inline std::string vk_str(const ImageLayout layout)
{
    switch (layout)
    {
#       define CASE_(x) case ImageLayout::x: return #x
        CASE_(Undefined);
        CASE_(ColorAttachment);
        CASE_(DepthAttachment);
        CASE_(ShaderReadOnly);
        CASE_(Present);
        CASE_(TransferSrc);
        CASE_(TransferDst);
#       undef CASE_
    }

    // Handle other values gracefully.
    ENSURE(false, "ImageLayout Enum not handled");
    std::ostringstream oss;
    oss << "ImageLayout(" << static_cast<std::underlying_type_t<ImageLayout>>(layout) << ")";
    return oss.str();
}

/**
 * @brief Converts a Format enums into a human-readable string literal.
 *
 * @see ImageLayout
 *
 * @param format The Image Format value.
 * @return A string matching the ImageLayout enum name (e.g., "RGBA8_SRGB").
 */
inline std::string vk_str(const Format format)
{
    switch (format)
    {
#       define CASE_(x) case Format::x: return #x
        CASE_(Undefined);
        CASE_(RGBA8_UNORM);
        CASE_(RGBA8_SRGB);
        CASE_(RGBA16_SFLOAT);
        CASE_(RG32_SFLOAT);
        CASE_(RGB32_SFLOAT);
        CASE_(RGBA32_SFLOAT);
        CASE_(RGBA32_UINT);
        CASE_(D32_SFLOAT);
#       undef CASE_
    }

    // Handle other values gracefully.
    ENSURE(false, "Format Enum not handled");
    std::ostringstream oss;
    oss << "Format(" << static_cast<std::underlying_type_t<Format>>(format) << ")";
    return oss.str();
}

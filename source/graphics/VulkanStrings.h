// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <string>
#include <sstream>

#include <volk.h>
#include <vulkan/vk_enum_string_helper.h>

#include "VulkanTypes.h"
#include "Ensure.h"

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
DEF_VK_STRING(VkObjectType);

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
 * @see Format
 *
 * @param format The Image Format value.
 * @return A string matching the Format enum name (e.g., "RGBA8_SRGB").
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

/**
 * @brief Converts a Vulkan object type enum into a friendly string.
 *
 * fstr - friendly string.
 *
 * @param inObjectType Vulkan object type enum value.
 * @return Friendly string for the Vulkan Object tupe enum name (e.g., "Buffer").
 */
inline std::string vk_fstr(const VkObjectType inObjectType)
{
    switch (inObjectType)
    {
        case VK_OBJECT_TYPE_BUFFER:
            return "Buffer";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:
            return "CommandBuffer";
        case VK_OBJECT_TYPE_COMMAND_POOL:
            return "CommandPool";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return "DescriptorPool";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
            return "DescriptorSet";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return "DescriptorSetLayout";
        case VK_OBJECT_TYPE_DEVICE:
            return "Device";
        case VK_OBJECT_TYPE_FENCE:
            return "Fence";
        case VK_OBJECT_TYPE_FRAMEBUFFER:
            return "Framebuffer";
        case VK_OBJECT_TYPE_IMAGE:
            return "Image";
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return "ImageView";
        case VK_OBJECT_TYPE_INSTANCE:
            return "Instance";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
            return "PhysicalDevice";
        case VK_OBJECT_TYPE_PIPELINE:
            return "Pipeline";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
            return "PipelineLayout";
        case VK_OBJECT_TYPE_RENDER_PASS:
            return "RenderPass";
        case VK_OBJECT_TYPE_SAMPLER:
            return "Sampler";
        case VK_OBJECT_TYPE_SEMAPHORE:
            return "Semaphore";
        case VK_OBJECT_TYPE_SHADER_MODULE:
            return "ShaderModule";
        case VK_OBJECT_TYPE_SURFACE_KHR:
            return "Surface";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
            return "Swapchain";
        default:
            break;
    }

    // Handle other values gracefully.
    ENSURE(false, "VkObjectType not handled");
    LOG_ERROR("VkObjectType not handled: {}", vk_str(inObjectType));
    return "Unkown";
}

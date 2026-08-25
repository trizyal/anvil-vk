// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "TextureLoader.h"

#include <stdexcept>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "GPUBuffer.h"
#include "VulkanResult.h"

namespace TextureLoader
{
    AnvilTexture LoadTexture(const std::string& filepath, VulkanContext& inContext, bool bIsSRGB)
    {
        std::string image_name = std::filesystem::path(filepath).filename().string();
        int tex_width, tex_height, tex_channels;
        stbi_uc* pixels = stbi_load(filepath.c_str(), &tex_width, &tex_height, &tex_channels, 4);
        if (!pixels) {
            throw std::runtime_error("Failed to load texture image: " + filepath);
        }

        VkDeviceSize image_size = tex_width * tex_height * 4;
        VkFormat texture_format = bIsSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

        // Calculate how many mip levels we need
        uint32_t mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        // CPU-visible staging buffer
        GPUBuffer staging_buffer;
        staging_buffer.createBuffer(
            inContext.allocator,
            inContext.device,
            pixels,
            image_size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            "TextureStagingBuffer");

        stbi_image_free(pixels);

        // GPU-Only Image
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = static_cast<uint32_t>(tex_width);
        image_info.extent.height = static_cast<uint32_t>(tex_height);
        image_info.extent.depth = 1;
        image_info.mipLevels = mip_levels;
        image_info.arrayLayers = 1;
        image_info.format = texture_format;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // Add TRANSFER_SRC_BIT so we can read from the image to generate the next mip level
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        AnvilTexture texture;
        if (vmaCreateImage(inContext.allocator, &image_info, &alloc_info, &texture.image, &texture.allocation, nullptr) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create texture image: " + image_name);
        }

        std::string texture_debug_name = "TextureImage: " + image_name;
        VulkanDebug::SetAutoName(inContext.device, texture.image, VK_OBJECT_TYPE_IMAGE, texture_debug_name.c_str());

        inContext.immediateSubmit([&](VkCommandBuffer cmd)
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = texture.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = mip_levels; // Transition all mips

            // Transition all mips to Transfer Destination
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            // Copy pixel data from buffer into Mip Level 0
            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0; // Only copy to level 0
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = {static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height), 1};

            vkCmdCopyBufferToImage(cmd, staging_buffer.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        });

        staging_buffer.destroyBuffer();

        // Create ImageView
        VkImageViewCreateInfo image_view_info{};
        image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_info.image = texture.image;
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_info.format = texture_format;
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_info.subresourceRange.baseMipLevel = 0;
        image_view_info.subresourceRange.levelCount = 1;
        image_view_info.subresourceRange.baseArrayLayer = 0;
        image_view_info.subresourceRange.layerCount = 1;

        std::string image_view_debug_name = "TextureImageView: " + image_name;
        CHECK(vkCreateImageView(inContext.device, &image_view_info, nullptr, &texture.imageView));
        VulkanDebug::SetAutoName(inContext.device, texture.imageView, VK_OBJECT_TYPE_IMAGE_VIEW, image_view_debug_name.c_str());

        // Create Sampler
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.anisotropyEnable = VK_TRUE;
        sampler_info.maxAnisotropy = inContext.physicalDeviceProperties.limits.maxSamplerAnisotropy;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        std::string sampler_debug_name = "TextureSampler: " + image_name;
        CHECK(vkCreateSampler(inContext.device, &sampler_info, nullptr, &texture.sampler));
        VulkanDebug::SetAutoName(inContext.device, texture.sampler, VK_OBJECT_TYPE_SAMPLER, sampler_debug_name.c_str());

        return texture;
    }

    AnvilTexture CreateSolidColorTexture(const uint8_t color[4], VulkanContext& inContext)
    {
        GPUBuffer staging_buffer;
        staging_buffer.createBuffer(
            inContext.allocator,
            inContext.device,
            color,
            sizeof(uint8_t) * 4,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent = {1, 1, 1};
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        AnvilTexture texture;
        if (vmaCreateImage(inContext.allocator, &image_info, &allocInfo, &texture.image, &texture.allocation, nullptr) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create solid texture image.");
        }

        std::string texture_debug_name = "SolidTextureImage";
        VulkanDebug::SetAutoName(inContext.device, texture.image, VK_OBJECT_TYPE_IMAGE, texture_debug_name.c_str());

        inContext.immediateSubmit([&](VkCommandBuffer cmd)
        {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = texture.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageExtent = {1, 1, 1};

            vkCmdCopyBufferToImage(cmd, staging_buffer.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        });

        staging_buffer.destroyBuffer();

        // Create ImageView
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = texture.image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        std::string imageViewDebugName = "TextureImageView";
        CHECK(vkCreateImageView(inContext.device, &view_info, nullptr, &texture.imageView));
        VulkanDebug::SetAutoName(inContext.device, texture.imageView, VK_OBJECT_TYPE_IMAGE_VIEW, imageViewDebugName.c_str());

        // Create Sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = inContext.physicalDeviceProperties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        std::string samplerDebugName = "TextureSampler";
        CHECK(vkCreateSampler(inContext.device, &samplerInfo, nullptr, &texture.sampler));
        VulkanDebug::SetAutoName(inContext.device, texture.sampler, VK_OBJECT_TYPE_SAMPLER, samplerDebugName.c_str());

        return texture;
    }
}

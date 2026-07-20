// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilSwapchain.h"

#include <iostream>

#include "GLFW/glfw3.h"

void AnvilSwapchain::initializeSwapchain(AnvilVulkanContext& inAnvilContext, VkExtent2D inExtent)
{
    std::cout << "Creating AnvilSwapchain" << std::endl;
    ptrContext = &inAnvilContext;

    vkb::SwapchainBuilder vkbSwapchainBuilder{
        ptrContext->anvilPhysicalDevice,
        ptrContext->anvilDevice,
        ptrContext->anvilSurface
    };

    vkbSwapchainBuilder.use_default_format_selection();
    vkbSwapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR); // vsync
    vkbSwapchainBuilder.set_desired_extent(inExtent.width, inExtent.height);
    vkb::Result<vkb::Swapchain> vkbSwapchainResult = vkbSwapchainBuilder.build();

    if (!vkbSwapchainResult)
    {
        // TODO: Print detailed error message for swapchain creation
        throw std::runtime_error("Failed to create swapchain.");
    }

    vkb::Swapchain vkbSwapchain = vkbSwapchainResult.value();
    anvilSwapchain = vkbSwapchain.swapchain;
    anvilExtent = vkbSwapchain.extent;
    anvilImageFormat = vkbSwapchain.image_format;

    anvilImages = vkbSwapchain.get_images().value();
    anvilImageViews = vkbSwapchain.get_image_views().value();

    createDepthAttachment();

    std::cout << "Finished creating AnvilSwapchain" << std::endl;
}

void AnvilSwapchain::recreateSwapchain(VkExtent2D inExtent)
{
    std::cout << "Re-Creating AnvilSwapchain" << std::endl;

    // Wait for GPU to finish
    vkDeviceWaitIdle(ptrContext->anvilDevice);

    // Save old swapchain handle
    VkSwapchainKHR oldSwapchain = anvilSwapchain;

    // Destroy old images views
    for (VkImageView imageView: anvilImageViews)
    {
        vkDestroyImageView(ptrContext->anvilDevice, imageView, nullptr);
    }
    anvilImageViews.clear();

    if (depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(ptrContext->anvilDevice, depthImageView, nullptr);
        vmaDestroyImage(ptrContext->anvilAllocator, depthImage, depthImageAllocation);
        depthImageView = VK_NULL_HANDLE;
    }

    // Build new swapchain using the old one
    vkb::SwapchainBuilder vkbSwapchainBuilder{
        ptrContext->anvilPhysicalDevice,
        ptrContext->anvilDevice,
        ptrContext->anvilSurface
    };

    vkbSwapchainBuilder.use_default_format_selection();
    vkbSwapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR); // vsync
    vkbSwapchainBuilder.set_desired_extent(inExtent.width, inExtent.height);

    vkbSwapchainBuilder.set_old_swapchain(oldSwapchain);

    vkb::Result<vkb::Swapchain> vkbSwapchainResult = vkbSwapchainBuilder.build();

    if (!vkbSwapchainResult)
    {
        // TODO: Print detailed error message for swapchain re-creation
        throw std::runtime_error("Failed to create swapchain.");
    }

    vkb::Swapchain vkbSwapchain = vkbSwapchainResult.value();
    anvilSwapchain = vkbSwapchain.swapchain;
    anvilExtent = vkbSwapchain.extent;
    anvilImageFormat = vkbSwapchain.image_format;

    anvilImages = vkbSwapchain.get_images().value();
    anvilImageViews = vkbSwapchain.get_image_views().value();

    createDepthAttachment();

    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(ptrContext->anvilDevice, oldSwapchain, nullptr);
    }

    std::cout << "Finished creating AnvilSwapchain" << std::endl;
}

void AnvilSwapchain::createDepthAttachment()
{
    // Create depth image via VMA
    VkImageCreateInfo depthImageInfo{};
    depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageInfo.format = depthFormat;
    depthImageInfo.extent = {anvilExtent.width, anvilExtent.height, 1};
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VmaAllocationCreateInfo depthAllocInfo{};
    depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    depthAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateImage(ptrContext->anvilAllocator, &depthImageInfo,&depthAllocInfo, &depthImage, &depthImageAllocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate depth image.");
    }

    // Create depth imageview
    VkImageViewCreateInfo depthImageViewInfo{};
    depthImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthImageViewInfo.image = depthImage;
    depthImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthImageViewInfo.format = depthFormat;
    depthImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthImageViewInfo.subresourceRange.baseMipLevel = 0;
    depthImageViewInfo.subresourceRange.levelCount = 1;
    depthImageViewInfo.subresourceRange.baseArrayLayer = 0;
    depthImageViewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(ptrContext->anvilDevice, &depthImageViewInfo, nullptr, &depthImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create depth image view.");
    }
}

void AnvilSwapchain::cleanup()
{
    for (VkImageView imageView: anvilImageViews)
    {
        vkDestroyImageView(ptrContext->anvilDevice, imageView, nullptr);
    }

    if (depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(ptrContext->anvilDevice, depthImageView, nullptr);
        vmaDestroyImage(ptrContext->anvilAllocator, depthImage, depthImageAllocation);
        depthImageView = VK_NULL_HANDLE;
    }

    vkDestroySwapchainKHR(ptrContext->anvilDevice, anvilSwapchain, nullptr);
    anvilSwapchain = VK_NULL_HANDLE;
}

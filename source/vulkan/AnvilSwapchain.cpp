// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilSwapchain.h"

#include <iostream>

#include <VkBootstrap.h>

#include "AnvilVulkanDebug.h"
#include "AnvilVulkanContext.h"

void AnvilSwapchain::initializeSwapchain(AnvilVulkanContext& inAnvilContext, VkExtent2D inExtent)
{
    std::cout << "Creating AnvilSwapchain" << std::endl;
    ptrAContext = &inAnvilContext;

    vkb::SwapchainBuilder vkbSwapchainBuilder{
        ptrAContext->anvilPhysicalDevice,
        ptrAContext->anvilDevice,
        ptrAContext->anvilSurface
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
    AnvilDebug::SetAutoName(inAnvilContext.anvilDevice, anvilSwapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "AnvilSwapchain");

    swapchainExtent = vkbSwapchain.extent;
    swapchainFormat = vkbSwapchain.image_format;

    swapchainImages = vkbSwapchain.get_images().value();
    swapchainImageViews = vkbSwapchain.get_image_views().value();

    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        std::string imageName = "SwapchainImage[" + std::to_string(i) + "]";
        AnvilDebug::SetAutoName(ptrAContext->anvilDevice, swapchainImages[i],
            VK_OBJECT_TYPE_IMAGE, imageName.c_str());
    }

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        std::string imageViewName = "SwapchainImageView[" + std::to_string(i) + "]";
        AnvilDebug::SetAutoName(ptrAContext->anvilDevice, swapchainImageViews[i],
            VK_OBJECT_TYPE_IMAGE_VIEW, imageViewName.c_str());
    }

    createDepthAttachment();

    std::cout << "Finished creating AnvilSwapchain" << std::endl;
}

void AnvilSwapchain::recreateSwapchain(AnvilVulkanContext& inAnvilContext, VkExtent2D inExtent)
{
    std::cout << "Re-Creating AnvilSwapchain" << std::endl;

    // Wait for GPU to finish
    vkDeviceWaitIdle(ptrAContext->anvilDevice);

    // Save old swapchain handle
    VkSwapchainKHR oldSwapchain = anvilSwapchain;
    [[maybe_unused]] VkFormat oldFormat = swapchainFormat;
    [[maybe_unused]] VkExtent2D oldExtent = swapchainExtent;

    // Destroy old images views
    for (VkImageView imageView: swapchainImageViews)
    {
        vkDestroyImageView(ptrAContext->anvilDevice, imageView, nullptr);
    }
    swapchainImageViews.clear();

    if (depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(ptrAContext->anvilDevice, depthImageView, nullptr);
        vmaDestroyImage(ptrAContext->anvilAllocator, depthImage, depthImageAllocation);
        depthImageView = VK_NULL_HANDLE;
    }

    // Build new swapchain using the old one
    vkb::SwapchainBuilder vkbSwapchainBuilder{
        ptrAContext->anvilPhysicalDevice,
        ptrAContext->anvilDevice,
        ptrAContext->anvilSurface
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
    AnvilDebug::SetAutoName(inAnvilContext.anvilDevice, anvilSwapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "AnvilSwapchain");

    swapchainExtent = vkbSwapchain.extent;
    swapchainFormat = vkbSwapchain.image_format;

    swapchainImages = vkbSwapchain.get_images().value();
    swapchainImageViews = vkbSwapchain.get_image_views().value();

    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        std::string imageName = "SwapchainImage[" + std::to_string(i) + "]";
        AnvilDebug::SetAutoName(ptrAContext->anvilDevice, swapchainImages[i],
            VK_OBJECT_TYPE_IMAGE, imageName.c_str());
    }

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        std::string imageViewName = "SwapchainImageView[" + std::to_string(i) + "]";
        AnvilDebug::SetAutoName(ptrAContext->anvilDevice, swapchainImageViews[i],
            VK_OBJECT_TYPE_IMAGE_VIEW, imageViewName.c_str());
    }

    createDepthAttachment();

    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(ptrAContext->anvilDevice, oldSwapchain, nullptr);
    }

    if (oldFormat != swapchainFormat)
    {
        std::cerr << "Swapchain format has changed!" << std::endl;
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
    depthImageInfo.extent = {swapchainExtent.width, swapchainExtent.height, 1};
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VmaAllocationCreateInfo depthAllocInfo{};
    depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    depthAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateImage(ptrAContext->anvilAllocator, &depthImageInfo,&depthAllocInfo, &depthImage, &depthImageAllocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate depth image.");
    }

    AnvilDebug::SetAutoName(ptrAContext->anvilDevice, depthImageView, VK_OBJECT_TYPE_IMAGE_VIEW, "SwapchainDepthImageView");

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

    if (vkCreateImageView(ptrAContext->anvilDevice, &depthImageViewInfo, nullptr, &depthImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create depth image view.");
    }

    AnvilDebug::SetAutoName(ptrAContext->anvilDevice, depthImage, VK_OBJECT_TYPE_IMAGE, "SwapchainDepthImage");
}

AnvilSwapchain::~AnvilSwapchain()
{
    for (VkImageView imageView: swapchainImageViews)
    {
        vkDestroyImageView(ptrAContext->anvilDevice, imageView, nullptr);
    }

    if (depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(ptrAContext->anvilDevice, depthImageView, nullptr);
        vmaDestroyImage(ptrAContext->anvilAllocator, depthImage, depthImageAllocation);
        depthImageView = VK_NULL_HANDLE;
    }

    vkDestroySwapchainKHR(ptrAContext->anvilDevice, anvilSwapchain, nullptr);
    anvilSwapchain = VK_NULL_HANDLE;
}

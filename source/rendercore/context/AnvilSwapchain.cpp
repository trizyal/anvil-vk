// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilSwapchain.h"

#include <iostream>
#include <sstream>

#include <VkBootstrap.h>

#include "AnvilVulkanDebug.h"
#include "AnvilVulkanContext.h"
#include "AnvilVulkanResult.h"

void AnvilSwapchain::initializeSwapchain(AnvilVulkanContext& inAnvilContext, VkExtent2D inExtent)
{
    std::cout << "Creating AnvilSwapchain" << std::endl;
    ptrAContext = &inAnvilContext;

    vkb::SwapchainBuilder vkb_swapchain_builder{
        ptrAContext->anvilPhysicalDevice,
        ptrAContext->anvilDevice,
        ptrAContext->anvilSurface
    };

    vkb_swapchain_builder.use_default_format_selection();
    vkb_swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR); // vsync
    vkb_swapchain_builder.set_desired_extent(inExtent.width, inExtent.height);
    vkb::Result<vkb::Swapchain> vkb_swapchain_result = vkb_swapchain_builder.build();

    if (!vkb_swapchain_result)
    {
        std::ostringstream error_stream;
        error_stream << "Failed to create Swapchain via vk-bootstrap:" << std::endl;
        error_stream << "  Primary error: " << vkb_swapchain_result.error().message() << std::endl;
        throw std::runtime_error(error_stream.str());
    }

    vkb::Swapchain vkb_swapchain = vkb_swapchain_result.value();
    anvilSwapchain = vkb_swapchain.swapchain;
    AnvilDebug::SetAutoName(inAnvilContext.anvilDevice, anvilSwapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "AnvilSwapchain");

    swapchainExtent = vkb_swapchain.extent;
    swapchainFormat = vkb_swapchain.image_format;

    swapchainImages = vkb_swapchain.get_images().value();
    swapchainImageViews = vkb_swapchain.get_image_views().value();

    // Setting debug names
    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        std::string image_name = "SwapchainImage[" + std::to_string(i) + "]";
        AnvilDebug::SetAutoName(ptrAContext->anvilDevice, swapchainImages[i],
            VK_OBJECT_TYPE_IMAGE, image_name.c_str());
    }

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        std::string image_view_name = "SwapchainImageView[" + std::to_string(i) + "]";
        AnvilDebug::SetAutoName(ptrAContext->anvilDevice, swapchainImageViews[i],
            VK_OBJECT_TYPE_IMAGE_VIEW, image_view_name.c_str());
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
    VkSwapchainKHR old_swapchain = anvilSwapchain;
    [[maybe_unused]] VkFormat old_format = swapchainFormat;
    [[maybe_unused]] VkExtent2D old_extent = swapchainExtent;

    // Destroy old images views
    for (VkImageView image_view: swapchainImageViews)
    {
        vkDestroyImageView(ptrAContext->anvilDevice, image_view, nullptr);
    }
    swapchainImageViews.clear();

    if (depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(ptrAContext->anvilDevice, depthImageView, nullptr);
        vmaDestroyImage(ptrAContext->anvilAllocator, depthImage, depthImageAllocation);
        depthImageView = VK_NULL_HANDLE;
    }

    // Build new swapchain using the old one
    vkb::SwapchainBuilder vkb_swapchain_builder{
        ptrAContext->anvilPhysicalDevice,
        ptrAContext->anvilDevice,
        ptrAContext->anvilSurface
    };

    vkb_swapchain_builder.use_default_format_selection();

    // TODO: Swapchain present mode should be configurable.
    vkb_swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR); // vsync
    vkb_swapchain_builder.set_desired_extent(inExtent.width, inExtent.height);

    vkb_swapchain_builder.set_old_swapchain(old_swapchain);

    vkb::Result<vkb::Swapchain> vkb_swapchain_result = vkb_swapchain_builder.build();

    if (!vkb_swapchain_result)
    {
        std::ostringstream error_stream;
        error_stream << "Failed to create Swapchain via vk-bootstrap:" << std::endl;
        error_stream << "  Primary error: " << vkb_swapchain_result.error().message() << std::endl;
        throw std::runtime_error(error_stream.str());
    }

    vkb::Swapchain vkb_swapchain = vkb_swapchain_result.value();
    anvilSwapchain = vkb_swapchain.swapchain;
    AnvilDebug::SetAutoName(inAnvilContext.anvilDevice, anvilSwapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "AnvilSwapchain");

    swapchainExtent = vkb_swapchain.extent;
    swapchainFormat = vkb_swapchain.image_format;

    swapchainImages = vkb_swapchain.get_images().value();
    swapchainImageViews = vkb_swapchain.get_image_views().value();

    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        std::string image_name = "SwapchainImage[" + std::to_string(i) + "]";
        AnvilDebug::SetAutoName(ptrAContext->anvilDevice, swapchainImages[i],
            VK_OBJECT_TYPE_IMAGE, image_name.c_str());
    }

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        std::string image_view_name = "SwapchainImageView[" + std::to_string(i) + "]";
        AnvilDebug::SetAutoName(ptrAContext->anvilDevice, swapchainImageViews[i],
            VK_OBJECT_TYPE_IMAGE_VIEW, image_view_name.c_str());
    }

    createDepthAttachment();

    if (old_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(ptrAContext->anvilDevice, old_swapchain, nullptr);
    }

    if (old_format != swapchainFormat)
    {
        std::cerr << "Swapchain format has changed!" << std::endl;
    }

    std::cout << "Finished creating AnvilSwapchain" << std::endl;
}

void AnvilSwapchain::createDepthAttachment()
{
    // Create depth image via VMA
    VkImageCreateInfo depth_image_info{};
    depth_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image_info.imageType = VK_IMAGE_TYPE_2D;
    depth_image_info.format = depthFormat;
    depth_image_info.extent = {swapchainExtent.width, swapchainExtent.height, 1};
    depth_image_info.mipLevels = 1;
    depth_image_info.arrayLayers = 1;
    depth_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VmaAllocationCreateInfo depth_alloc_info{};
    depth_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    depth_alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    CHECK(vmaCreateImage(ptrAContext->anvilAllocator, &depth_image_info,&depth_alloc_info, &depthImage, &depthImageAllocation, nullptr));

    AnvilDebug::SetAutoName(ptrAContext->anvilDevice, depthImageView, VK_OBJECT_TYPE_IMAGE_VIEW, "SwapchainDepthImageView");

    // Create depth imageview
    VkImageViewCreateInfo depth_image_view_info{};
    depth_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_image_view_info.image = depthImage;
    depth_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_image_view_info.format = depthFormat;
    depth_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_view_info.subresourceRange.baseMipLevel = 0;
    depth_image_view_info.subresourceRange.levelCount = 1;
    depth_image_view_info.subresourceRange.baseArrayLayer = 0;
    depth_image_view_info.subresourceRange.layerCount = 1;

    CHECK(vkCreateImageView(ptrAContext->anvilDevice, &depth_image_view_info, nullptr, &depthImageView));

    AnvilDebug::SetAutoName(ptrAContext->anvilDevice, depthImage, VK_OBJECT_TYPE_IMAGE, "SwapchainDepthImage");
}

AnvilSwapchain::~AnvilSwapchain()
{
    for (VkImageView image_view: swapchainImageViews)
    {
        vkDestroyImageView(ptrAContext->anvilDevice, image_view, nullptr);
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

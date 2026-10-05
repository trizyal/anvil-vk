// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Swapchain.h"

#include <sstream>

#include <VkBootstrap.h>

#include "DebugNames.h"
#include "Logger.h"
#include "Trace.h"
#include "VulkanContext.h"
#include "VulkanResult.h"

void Swapchain::initializeSwapchain(VulkanContext& inAnvilContext, VkExtent2D inExtent)
{
    SCOPE_CPU;
    LOG_TRACE("Creating Swapchain");

    pContext = &inAnvilContext;

    vkb::SwapchainBuilder vkb_swapchain_builder{
        pContext->physicalDevice,
        pContext->device,
        pContext->surface
    };

    vkb_swapchain_builder.use_default_format_selection();
    // vkb_swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR); // vsync
    vkb_swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR); // no vsync
    vkb_swapchain_builder.set_desired_extent(inExtent.width, inExtent.height);
    vkb::Result<vkb::Swapchain> vkb_swapchain_result = vkb_swapchain_builder.build();
    VKB_CHECK(vkb_swapchain_result);

    vkb::Swapchain vkb_swapchain = vkb_swapchain_result.value();
    anvilSwapchain = vkb_swapchain.swapchain;
    SET_DNAME_HERE(inAnvilContext.device, anvilSwapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "AnvilSwapchain");

    swapchainExtent = vkb_swapchain.extent;
    swapchainFormat = static_cast<Format>(vkb_swapchain.image_format);

    swapchainImages = vkb_swapchain.get_images().value();
    swapchainImageViews = vkb_swapchain.get_image_views().value();

    swapchainImageLayouts.assign(swapchainImages.size(), ImageLayout::Undefined);

    // Setting debug names
    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        SET_DNAME_HERE(pContext->device, swapchainImages[i], VK_OBJECT_TYPE_IMAGE, ( "SwapchainImage" + std::to_string(i)).c_str());
    }

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        SET_DNAME_HERE(pContext->device, swapchainImageViews[i], VK_OBJECT_TYPE_IMAGE_VIEW, ("SwapchainImageView" + std::to_string(i)).c_str());
    }

    createDepthAttachment();

    LOG_TRACE("Finished creating AnvilSwapchain");
}

void Swapchain::recreateSwapchain(VkExtent2D inExtent)
{
    SCOPE_CPU;
    LOG_TRACE("Re-creating AnvilSwapchain");

    // Wait for GPU to finish
    vkDeviceWaitIdle(pContext->device);

    // Save old swapchain handle
    VkSwapchainKHR old_swapchain = anvilSwapchain;
    [[maybe_unused]] Format old_format = swapchainFormat;
    [[maybe_unused]] VkExtent2D old_extent = swapchainExtent;

    // Destroy old images views
    for (VkImageView image_view: swapchainImageViews)
    {
        vkDestroyImageView(pContext->device, image_view, nullptr);
    }
    swapchainImageViews.clear();

    if (depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(pContext->device, depthImageView, nullptr);
        vmaDestroyImage(pContext->allocator, depthImage, depthImageAllocation);
        depthImageView = VK_NULL_HANDLE;
    }

    // Build new swapchain using the old one
    vkb::SwapchainBuilder vkb_swapchain_builder{
        pContext->physicalDevice,
        pContext->device,
        pContext->surface
    };

    vkb_swapchain_builder.use_default_format_selection();

    // TODO: Swapchain present mode should be configurable.
    // vkb_swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR); // vsync
    vkb_swapchain_builder.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR); // vsync
    vkb_swapchain_builder.set_desired_extent(inExtent.width, inExtent.height);

    vkb_swapchain_builder.set_old_swapchain(old_swapchain);

    vkb::Result<vkb::Swapchain> vkb_swapchain_result = vkb_swapchain_builder.build();
    VKB_CHECK(vkb_swapchain_result);

    vkb::Swapchain vkb_swapchain = vkb_swapchain_result.value();
    anvilSwapchain = vkb_swapchain.swapchain;
    SET_DNAME_HERE(pContext->device, anvilSwapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "AnvilSwapchain");

    swapchainExtent = vkb_swapchain.extent;
    swapchainFormat = static_cast<Format>(vkb_swapchain.image_format);

    swapchainImages = vkb_swapchain.get_images().value();
    swapchainImageViews = vkb_swapchain.get_image_views().value();

    swapchainImageLayouts.assign(swapchainImages.size(), ImageLayout::Undefined);

    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
        SET_DNAME_HERE(pContext->device, swapchainImages[i],
            VK_OBJECT_TYPE_IMAGE, ("SwapchainImage[" + std::to_string(i) + "]").c_str());
    }

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        SET_DNAME_HERE(pContext->device, swapchainImageViews[i],
            VK_OBJECT_TYPE_IMAGE_VIEW, ("SwapchainImageView[" + std::to_string(i) + "]").c_str());
    }

    createDepthAttachment();

    if (old_swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(pContext->device, old_swapchain, nullptr);
    }

    if (old_format != swapchainFormat)
    {
        LOG_ERROR("Swapchain format has changed!");
    }

    LOG_TRACE("Finished re-creating AnvilSwapchain");
}

void Swapchain::createDepthAttachment()
{
    SCOPE_CPU;

    // Create depth image via VMA
    VkImageCreateInfo depth_image_info{};
    depth_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image_info.imageType = VK_IMAGE_TYPE_2D;
    depth_image_info.format = static_cast<VkFormat>(depthFormat);
    depth_image_info.extent = {swapchainExtent.width, swapchainExtent.height, 1};
    depth_image_info.mipLevels = 1;
    depth_image_info.arrayLayers = 1;
    depth_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VmaAllocationCreateInfo depth_alloc_info{};
    depth_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    depth_alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    CHECK(vmaCreateImage(pContext->allocator, &depth_image_info,&depth_alloc_info, &depthImage, &depthImageAllocation, nullptr));
    SET_DNAME_HERE(pContext->device, depthImageView, VK_OBJECT_TYPE_IMAGE_VIEW, "SwapchainDepthImageView");
    SET_VMA_DNAME_HERE(pContext->allocator, depthImageAllocation, "SwapchainDepthImageViewAllocation");

    // Create depth imageview
    VkImageViewCreateInfo depth_image_view_info{};
    depth_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_image_view_info.image = depthImage;
    depth_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_image_view_info.format = static_cast<VkFormat>(depthFormat);
    depth_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_view_info.subresourceRange.baseMipLevel = 0;
    depth_image_view_info.subresourceRange.levelCount = 1;
    depth_image_view_info.subresourceRange.baseArrayLayer = 0;
    depth_image_view_info.subresourceRange.layerCount = 1;

    CHECK(vkCreateImageView(pContext->device, &depth_image_view_info, nullptr, &depthImageView));
    SET_DNAME_HERE(pContext->device, depthImage, VK_OBJECT_TYPE_IMAGE, "SwapchainDepthImage");
}

Swapchain::~Swapchain()
{
    if (pContext)
    {
        for (VkImageView image_view: swapchainImageViews)
        {
            vkDestroyImageView(pContext->device, image_view, nullptr);
        }

        if (depthImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(pContext->device, depthImageView, nullptr);
            vmaDestroyImage(pContext->allocator, depthImage, depthImageAllocation);
            depthImageView = VK_NULL_HANDLE;
        }

        vkDestroySwapchainKHR(pContext->device, anvilSwapchain, nullptr);
        anvilSwapchain = VK_NULL_HANDLE;
    }
}

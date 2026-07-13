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

    if (oldSwapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(ptrContext->anvilDevice, oldSwapchain, nullptr);
    }

    std::cout << "Finished creating AnvilSwapchain" << std::endl;
}

void AnvilSwapchain::cleanup()
{
    for (VkImageView imageView: anvilImageViews)
    {
        vkDestroyImageView(ptrContext->anvilDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(ptrContext->anvilDevice, anvilSwapchain, nullptr);
    anvilSwapchain = VK_NULL_HANDLE;
}

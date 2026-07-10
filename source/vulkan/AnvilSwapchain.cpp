// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilSwapchain.h"

#include <iostream>

#include "GLFW/glfw3.h"

void AnvilSwapchain::initializeSwapchain(AnvilVulkanContext& inAnvilContext, VkExtent2D inExtent)
{
    std::cout << "Creating AnvilSwapchain" << std::endl;
    ptrContext = &inAnvilContext;
    buildSwapchainInternal(inExtent);
    std::cout << "Finished creating AnvilSwapchain" << std::endl;
}

void AnvilSwapchain::recreateSwapchain(VkExtent2D inExtent)
{
    std::cout << "Re-Creating AnvilSwapchain" << std::endl;
    // Wait for GPU to finish
    vkDeviceWaitIdle(ptrContext->anvilDevice);
    cleanup();

    buildSwapchainInternal(inExtent);
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

void AnvilSwapchain::buildSwapchainInternal(VkExtent2D inExtent)
{
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
        // TODO: Print detailed error message
        throw std::runtime_error("Failed to create swapchain.");
    }

    vkb::Swapchain vkbSwapchain = vkbSwapchainResult.value();
    anvilSwapchain = vkbSwapchain.swapchain;
    anvilExtent = vkbSwapchain.extent;
    anvilImageFormat = vkbSwapchain.image_format;

    anvilImages = vkbSwapchain.get_images().value();
    anvilImageViews = vkbSwapchain.get_image_views().value();
}

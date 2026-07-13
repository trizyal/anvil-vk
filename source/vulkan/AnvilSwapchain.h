// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SWAPCHAIN_H
#define ANVIL_VK_SWAPCHAIN_H

#include <vector>

#include <VkBootstrap.h>

#include "AnvilVulkanContext.h"

class AnvilSwapchain
{
private:
    AnvilVulkanContext *ptrContext = nullptr;

public:
    VkSwapchainKHR anvilSwapchain = VK_NULL_HANDLE;
    VkExtent2D anvilExtent;
    VkFormat anvilImageFormat;

    std::vector<VkImage> anvilImages;
    std::vector<VkImageView> anvilImageViews;

    void initializeSwapchain(AnvilVulkanContext& inAnvilContext, VkExtent2D inExtent);
    void recreateSwapchain(VkExtent2D inExtent);
    void cleanup();
};

#endif //ANVIL_VK_SWAPCHAIN_H

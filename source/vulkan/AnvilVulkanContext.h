// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_VULKANCONTEXT_H
#define ANVIL_VK_VULKANCONTEXT_H

#include <volk.h>
#include <vk_mem_alloc.h>

#include "AnvilDeletionQueue.h"

class AnvilWindow;

class AnvilVulkanContext
{
public:
    VkInstance anvilInstance;
    VkDebugUtilsMessengerEXT anvilDebugMessenger;
    VkPhysicalDevice anvilPhysicalDevice;
    VkDevice anvilDevice;
    VkSurfaceKHR anvilSurface;

    VkQueue anvilGraphicsQueue;
    uint32_t anvilGraphicsQueueFamily;

    VmaAllocator anvilAllocator;

    AnvilDeletionQueue anvilDeletionQueue;

    void initializeVulkanContext(AnvilWindow& inWindow);
    void destroyVulkanContext();
};

#endif //ANVIL_VK_VULKANCONTEXT_H

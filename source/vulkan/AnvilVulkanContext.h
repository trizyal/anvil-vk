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
    VkInstance anvilInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT anvilDebugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice anvilPhysicalDevice = VK_NULL_HANDLE;
    VkDevice anvilDevice = VK_NULL_HANDLE;
    VkSurfaceKHR anvilSurface = VK_NULL_HANDLE;

    VkQueue anvilGraphicsQueue = VK_NULL_HANDLE;
    uint32_t anvilGraphicsQueueIndex = 0;

    VmaAllocator anvilAllocator = VK_NULL_HANDLE;

    AnvilDeletionQueue anvilDeletionQueue;

    void initializeVulkanContext(AnvilWindow& inWindow);
    void destroyVulkanContext();
};

#endif //ANVIL_VK_VULKANCONTEXT_H

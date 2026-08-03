// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_VULKANCONTEXT_H
#define ANVIL_VK_VULKANCONTEXT_H

/**
 * @file AnvilVulkanContext.h
 * @brief Core Vulkan initialization, device management, and GPU memory allocator context.
 */

#include <functional>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "AnvilDeletionQueue.h"

class AnvilWindow;

/**
 * @brief Root Vulkan context managing the instance, logical device, allocator and submission queues.
 *
 * Owns the primary Vulkan API handles required to interface with the GPU. Designed as a
 * pinned, non-copyable, and non-movable object so that child resources (buffers, shaders, meshes)
 * can safely cache permanent pointers or references to this context without risking dangling pointers.
 *
 * @note This class is non-copyable and non-movable.
 */
class AnvilVulkanContext
{
public:
    AnvilVulkanContext() = default;
    ~AnvilVulkanContext();

    AnvilVulkanContext(const AnvilVulkanContext&) = delete;
    AnvilVulkanContext& operator=(const AnvilVulkanContext&) = delete;
    AnvilVulkanContext(AnvilVulkanContext&&) = delete;
    AnvilVulkanContext& operator=(AnvilVulkanContext&&) = delete;

    VkInstance anvilInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT anvilDebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR anvilSurface = VK_NULL_HANDLE;
    VkPhysicalDevice anvilPhysicalDevice = VK_NULL_HANDLE;
    VkDevice anvilDevice = VK_NULL_HANDLE;

    VkQueue anvilGraphicsQueue = VK_NULL_HANDLE;
    uint32_t anvilGraphicsQueueIndex = 0;

    VmaAllocator anvilAllocator = VK_NULL_HANDLE;

    void initializeVulkanContext(AnvilWindow& inWindow);

    // MEMBERS FOR IMMEDIATE SUBMIT
    VkCommandPool uploadCommandPool = VK_NULL_HANDLE;
    VkFence uploadFence = VK_NULL_HANDLE;

    // Takes a lambda containing Vulkan commands and executes them immediately
    void immediateSubmit(std::function<void(VkCommandBuffer inCmd)>&& callbackFunction);
};

#endif //ANVIL_VK_VULKANCONTEXT_H

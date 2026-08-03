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
    /**
     * @brief Constructs an uninitialized Vulkan context container.
     */
    AnvilVulkanContext() = default;

    /**
     * @brief Destroy all owned Vulkan handles and destroys the C++ wrapper object.
     */
    ~AnvilVulkanContext();

    AnvilVulkanContext(const AnvilVulkanContext&) = delete;
    AnvilVulkanContext& operator=(const AnvilVulkanContext&) = delete;
    AnvilVulkanContext(AnvilVulkanContext&&) = delete;
    AnvilVulkanContext& operator=(AnvilVulkanContext&&) = delete;

    /** Vulkan API instance handle. */
    VkInstance anvilInstance = VK_NULL_HANDLE;

    /** Dedicated debug callback messenger for validation layers. */
    VkDebugUtilsMessengerEXT anvilDebugMessenger = VK_NULL_HANDLE;

    /** Window system integration (WSI) rendering surface. */
    VkSurfaceKHR anvilSurface = VK_NULL_HANDLE;

    /** Selected GPU physical device handle. */
    VkPhysicalDevice anvilPhysicalDevice = VK_NULL_HANDLE;

    /** Logical GPU device handle used for resource creation. */
    VkDevice anvilDevice = VK_NULL_HANDLE;

    /** Queue handle for graphics and transfer command submissions. */
    VkQueue anvilGraphicsQueue = VK_NULL_HANDLE;

    /** Queue family index corresponding to anvilGraphicsQueue. */
    uint32_t anvilGraphicsQueueIndex = 0;

    /** Vulkan Memory Allocator (VMA) instance for GPU memory management. */
    VmaAllocator anvilAllocator = VK_NULL_HANDLE;

    /**
     * @brief Initializes the Vulkan instance, device, VMA, and rendering surface.
     * @param inWindow Reference to the application window used to create the Vulkan surface.
     *
     * @throws std::runtime_error If Vulkan instance creation, device selection, or VMA initialization fails.
     */
    void initializeVulkanContext(AnvilWindow& inWindow);

    // MEMBERS FOR IMMEDIATE SUBMIT
private:
    /** Pointer to the application window. */
    AnvilWindow* ptrAWindow = nullptr;

    /** Dedicated command pool for immediate CPU-to-GPU transfer submissions. */
    VkCommandPool uploadCommandPool = VK_NULL_HANDLE;

    /** Synchronization fence used to wait for immediateSubmit() completion. */
    VkFence uploadFence = VK_NULL_HANDLE;

public:
    // Takes a lambda containing Vulkan commands and executes them immediately
    /**
     * @brief Allocates a temporary command buffer, records a callback, and submits it immediately to the GPU.
     *
     * Blocks CPU execution until the submitted commands finish executing on the graphics queue.
     * Ideal for staging buffer uploads, shader layout transitions, or one-off GPU initialization commands.
     *
     * @param callbackFunction Lambda or functor receiving an active `VkCommandBuffer` to record commands into.
     *
     * @throws std::runtime_error If command buffer allocation, submission, or fence waiting fails.
     */
    void immediateSubmit(std::function<void(VkCommandBuffer inCmd)>&& callbackFunction);

private:
    /**
     * @brief Internal helper to initialize Volk and create the Vulkan instance via vk-bootstrap.
     *
     * @throws std::runtime_error If Volk initialization or instance creation fails.
     */
    void createInstance();
};

#endif //ANVIL_VK_VULKANCONTEXT_H

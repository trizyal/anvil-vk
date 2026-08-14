// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_SWAPCHAIN_H
#define ANVIL_VK_SWAPCHAIN_H

/**
 * @file VulkanSwapchain.h
 * @brief Encapsulates a Vulkan swapchain, color image views, and a dedicated depth buffer attachment.
 */

#include <vector>

#include <volk.h>
#include <vk_mem_alloc.h>

class VulkanContext;

/**
 * @brief Manages the swapchain, presentation images, and a depth attachment for rendering.
 *
 * Handles creation and dynamic window-resize recreation of Vulkan swapchain along with its associated
 * color and depth image views.
 *
 * @note This class is non-copyable and non-movable.
 *
 * @todo Need to make presentation mode configurable.
 *
 * @warning Will need to decouple the depth from swapchain when moving to
 * deferred rendering, shadow mapping or post processing.
 */
class Swapchain
{
public:
    /**
     * @brief Constructs an uninitialized Vulkan swapchain container.
     */
    Swapchain() = default;

    /**
     * @brief Destroys the swapchain, all presentation views, and the depth attachment.
     */
    ~Swapchain();


    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(Swapchain&&) = delete;
    Swapchain& operator=(Swapchain&&) = delete;

    /** Underlying Vulkan swapchain handle. */
    VkSwapchainKHR anvilSwapchain = VK_NULL_HANDLE;

    /** Resolution (width and height in pixels) of the swapchain images. */
    VkExtent2D swapchainExtent{};

    /** Pixel format selected for presentation images (e.g., B8G8R8A8_SRGB). */
    VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;

    /** Presentation image handles retrieved from the Vulkan swapchain. */
    std::vector<VkImage> swapchainImages;

    /** 2D image views created for each presentation image in anvilImages. */
    std::vector<VkImageView> swapchainImageViews;

    // ------------------------------------------------------------------------
    // Depth Attachment Properties
    // ------------------------------------------------------------------------
    /** Set here itself because swapchain does not actually care about depth. */
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    /** GPU image handle for the depth attachment. */
    VkImage depthImage = VK_NULL_HANDLE;

    /** 2D image view used to bind the depth image to rendering pipelines. */
    VkImageView depthImageView = VK_NULL_HANDLE;

    /** VMA memory allocation backing the depth image. */
    VmaAllocation depthImageAllocation = VK_NULL_HANDLE;

private:
    /** Cached pointer to the parent Vulkan context. */
    VulkanContext* pContext = nullptr;

public:
    /**
     * @brief Initializes the Vulkan swapchain, color image views, and depth attachment.
     *
     * Caches the provided context pointer internally for cleanup and subsequent recreations.
     *
     * @param inAnvilContext Reference to the root Vulkan context, for surface and device.
     * @param inExtent Pixel dimensions of the rendering surface.
     *
     * @throws std::runtime_error If swapchain, image view, or depth buffer creation fails.
     */
    void initializeSwapchain(VulkanContext& inAnvilContext, VkExtent2D inExtent);

    /**
     * @brief Recreates the swapchain and its attachments to handle window resizing or surface changes.
     *
     * Safely destroys existing swapchain image views and depth attachments before allocating new
     * resources matching the updated extent.
     *
     * @param inAnvilContext Reference to the root Vulkan context, for surface and device.
     * @param inExtent Pixel dimensions of the rendering surface.
     *
     * @throws std::runtime_error If swapchain, image view, or depth buffer creation fails.
     */
    void recreateSwapchain(VulkanContext& inAnvilContext, VkExtent2D inExtent);

private:
    /**
     * @brief Internal helper to allocate and create the depth image and image view.
     *
     * Allocates a device-local image via VMA matching the current swapchain resolution
     * and creates a depth aspect image view for pipeline attachment.
     *
     * @throws std::runtime_error If VMA image allocation or image view creation fails.
     */
    void createDepthAttachment();
};

#endif //ANVIL_VK_SWAPCHAIN_H

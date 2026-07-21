// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_UIRENDERER_H
#define ANVIL_VK_UIRENDERER_H

#include <volk.h>
#include <GLFW/glfw3.h>

#include "AnvilVulkanDebug.h"

class AnvilVulkanContext;
class AnvilSwapchain;

class AnvilUIRenderer
{
public:
    AnvilUIRenderer() = default;
    ~AnvilUIRenderer() = default;

    VkFormat colorFormat;
    VkFormat depthFormat;

private:
    VkDescriptorPool imguiPool = VK_NULL_HANDLE;

public:
    bool initializeUIRenderer(AnvilVulkanContext* inContext, GLFWwindow* inWindow, AnvilSwapchain* inSwapchain);
    void destroy(AnvilVulkanContext* inContext);

    // Call this at the very beginning of your frame loop (before rendering)
    void beginUIFrame();

    // Call this INSIDE your command buffer recording, while Dynamic Rendering is active!
    void recordUICommands(VkCommandBuffer cmdBuffer);

    // Call this at the very end of your frame (after vkQueueSubmit) to handle multi-viewports
    void endUIFrame();

private:
    void createDescriptorPool(VkDevice inDevice ANVIL_DEBUG_DECL());
};

#endif //ANVIL_VK_UIRENDERER_H

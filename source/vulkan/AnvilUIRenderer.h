// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_UIRENDERER_H
#define ANVIL_VK_UIRENDERER_H

#include <volk.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "AnvilVulkanDebug.h"

class AnvilVulkanContext;
class AnvilSwapchain;

class AnvilUIRenderer
{
public:
    AnvilUIRenderer() = default;
    ~AnvilUIRenderer();

    VkFormat colorFormat;
    VkFormat depthFormat;

private:
    AnvilVulkanContext* ptrAContext = nullptr;

    VkDescriptorPool imguiPool = VK_NULL_HANDLE;

public:
    bool initializeUIRenderer(AnvilVulkanContext* inContext, GLFWwindow* inWindow, AnvilSwapchain* inSwapchain);

    static void BeginUIFrame();
    static void RecordUICommands(VkCommandBuffer cmdBuffer);
    static void EndUIFrame();

    static void DrawDebugAxis(const glm::mat4& viewMatrix);

private:
    void createDescriptorPool(VkDevice inDevice ANVIL_DEBUG_DECL());
};

#endif //ANVIL_VK_UIRENDERER_H

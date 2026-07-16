// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_RENDERER_H
#define ANVIL_VK_RENDERER_H

#include <functional>

#include "AnvilSwapchain.h"

struct AnvilFrame
{
    VkCommandPool cmdPool;
    VkCommandBuffer cmdBuffer;

    // Sync objects
    VkSemaphore imageAvailableSemaphore; // Image is ready to render to
    VkFence frameDoneFence; // CPU waits for GPU to finish this frame
};

constexpr uint32_t FRAMES_IN_FLIGHT = 2;

class AnvilRenderer
{
private:
    AnvilVulkanContext* ptrAContext = nullptr;
    AnvilSwapchain* ptrASwapchain = nullptr;

    AnvilFrame anvilFrames[FRAMES_IN_FLIGHT];
    uint32_t anvilFrameIndex = 0;

    // Rendering is finished, ready to present
    std::vector<VkSemaphore> renderFinishedSemaphores;

    bool recreateSwapchain = false;

    AnvilFrame& getCurrentFrame();
    void setupCommandBuffers();
    void setupSyncStructures();
    void transitionImageLayout(VkCommandBuffer inCmd, VkImage inImage,
        VkImageLayout oldLayout, VkImageLayout newLayout);

public:
    AnvilRenderer() = default;

    void initializeRenderer(AnvilVulkanContext* inAnvilContext, AnvilSwapchain* inAnvilSwapchain);
    void cleanup();

    void drawFrame(AnvilWindow& inWindow);
    void drawFrame(AnvilWindow& inWindow, const std::function<void(VkCommandBuffer, VkExtent2D)>& drawCallback);
};

#endif //ANVIL_VK_RENDERER_H

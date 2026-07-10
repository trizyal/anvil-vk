// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_RENDERER_H
#define ANVIL_VK_RENDERER_H

#include "AnvilSwapchain.h"

struct AnvilFrame
{
    VkCommandPool cmdPool;
    VkCommandBuffer cmdBuffer;

    // Sync objects
    VkSemaphore swapchainSemaphore; // Image is ready to render to
    VkSemaphore renderSemaphore; // Render is finished, ready to present
    VkFence renderFence; // CPU waits for GPU to finish this frame
};

constexpr uint32_t FRAMES_IN_FLIGHT = 2;

class AnvilRenderer
{
private:
    AnvilVulkanContext* ptrAContext = nullptr;
    AnvilSwapchain* ptrASwapchain = nullptr;

    AnvilFrame anvilFrames[FRAMES_IN_FLIGHT];
    uint32_t anvilFrameNumber = 0;
    std::vector<VkFence> imagesInFlight;

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
};

#endif //ANVIL_VK_RENDERER_H

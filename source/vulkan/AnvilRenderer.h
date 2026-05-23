#ifndef ANVIL_VK_RENDERER_H
#define ANVIL_VK_RENDERER_H

#include "AnvilSwapchain.h"

struct AnvilFrame
{
    VkCommandPool anvilCommandPool;
    VkCommandBuffer anvilCommandBuffer;

    // Sync objects
    VkSemaphore anvilSwapchainSemaphore; // Image is ready to render to
    VkSemaphore anvilRenderSemaphore; // Render is finished, ready to present
    VkFence anvilRenderFence; // CPU waits for GPU to finish this frame
};

constexpr uint32_t FRAMES_IN_FLIGHT = 2;

class AnvilRenderer
{
private:
    AnvilVulkanContext* ptrAContext;
    AnvilSwapchain* ptrASwapchain;

    AnvilFrame anvilFrames[FRAMES_IN_FLIGHT];
    uint32_t anvilFrameNumber = 0;

    AnvilFrame& getCurrentFrame();
    void setupCommandBuffers();
    void setupSyncStructures();

public:
    void initialise(AnvilVulkanContext* inAnvilContext, AnvilSwapchain* inAnvilSwapchain);
    void cleanup();

    void drawFrame();
};

#endif //ANVIL_VK_RENDERER_H

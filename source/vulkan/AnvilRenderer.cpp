#include "AnvilRenderer.h"

#include <iostream>
#include <stdexcept>

void AnvilRenderer::initialise(AnvilVulkanContext* inAnvilContext, AnvilSwapchain* inAnvilSwapchain)
{
    this->ptrAContext = inAnvilContext;
    this->ptrASwapchain = inAnvilSwapchain;

    setupCommandBuffers();
    setupSyncStructures();
}

void AnvilRenderer::cleanup()
{
    // Wait for GPU
    vkDeviceWaitIdle(ptrAContext->anvilDevice);

    for (const AnvilFrame& anvilFrame : anvilFrames)
    {
        vkDestroySemaphore(ptrAContext->anvilDevice, anvilFrame.anvilSwapchainSemaphore, nullptr);
        vkDestroySemaphore(ptrAContext->anvilDevice, anvilFrame.anvilRenderSemaphore, nullptr);
        vkDestroyFence(ptrAContext->anvilDevice, anvilFrame.anvilRenderFence, nullptr);
        vkDestroyCommandPool(ptrAContext->anvilDevice, anvilFrame.anvilCommandPool, nullptr);
    }
}

void AnvilRenderer::drawFrame()
{
    AnvilFrame& currentFrame = getCurrentFrame();

    // Wait for previous frame
}

void AnvilRenderer::setupCommandBuffers()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = ptrAContext->anvilGraphicsQueueFamily;

    for (AnvilFrame& anvilFrame : anvilFrames)
    {
        if (vkCreateCommandPool(ptrAContext->anvilDevice, &poolInfo, nullptr, &anvilFrame.anvilCommandPool) != VK_SUCCESS)
        {
            // TODO: Provide better error message
            throw std::runtime_error("Failed to create command pool.");
        }

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = anvilFrame.anvilCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(ptrAContext->anvilDevice, &allocInfo, &anvilFrame.anvilCommandBuffer) != VK_SUCCESS)
        {
            // TODO: Provide better error message
            throw std::runtime_error("Failed to allocate command buffer.");
        }
    }
}

void AnvilRenderer::setupSyncStructures()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (AnvilFrame& anvilFrame : anvilFrames)
    {
        // TODO: Provide better error messages for Semaphores and Fences
        if (vkCreateSemaphore(ptrAContext->anvilDevice, &semaphoreInfo, nullptr, &anvilFrame.anvilSwapchainSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swapchain semaphore.");
        }
        if (vkCreateSemaphore(ptrAContext->anvilDevice, &semaphoreInfo, nullptr, &anvilFrame.anvilRenderSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render semaphore.");
        }
        if (vkCreateFence(ptrAContext->anvilDevice, &fenceInfo, nullptr, &anvilFrame.anvilRenderFence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render fence.");
        }
    }
}

AnvilFrame& AnvilRenderer::getCurrentFrame()
{
    return anvilFrames[anvilFrameNumber % FRAMES_IN_FLIGHT];
}

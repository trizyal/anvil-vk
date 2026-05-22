#include "AnvilRenderer.h"

#include <iostream>
#include <stdexcept>

void AnvilRenderer::initialise(AnvilVulkanContext* inAnvilContext, AnvilSwapchain* inAnvilSwapchain)
{
    this->anvilContext = inAnvilContext;
    this->anvilSwapchain = inAnvilSwapchain;

    setupCommandBuffers();
    setupSyncStructures();
}

void AnvilRenderer::cleanup()
{

}

void AnvilRenderer::drawFrame()
{

}

void AnvilRenderer::setupCommandBuffers()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = anvilContext->anvilGraphicsQueueFamily;

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateCommandPool(anvilContext->anvilDevice, &poolInfo, nullptr, &anvilFrames[i].anvilCommandPool) != VK_SUCCESS)
        {
            // TODO: Provide better error message
            throw std::runtime_error("Failed to create command pool.");
        }

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = anvilFrames[i].anvilCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(anvilContext->anvilDevice, &allocInfo, &anvilFrames[i].anvilCommandBuffer) != VK_SUCCESS)
        {
            // TODO: Provide better error message
            throw std::runtime_error("Failed to allocate command buffer.");
        }
    }
}

void AnvilRenderer::setupSyncStructures()
{

}

AnvilFrame& AnvilRenderer::getCurrentFrame()
{
    return anvilFrames[anvilFrameNumber % FRAMES_IN_FLIGHT];
}

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilRenderer.h"

#include <iostream>
#include <stdexcept>

#include "AnvilWindow.h"

void AnvilRenderer::initializeRenderer(AnvilVulkanContext* inAnvilContext, AnvilSwapchain* inAnvilSwapchain)
{
    std::cout << "Initializing AnvilRenderer" << std::endl;
    this->ptrAContext = inAnvilContext;
    this->ptrASwapchain = inAnvilSwapchain;

    setupCommandBuffers();
    setupSyncStructures();

    std::cout << "Finished Initializing AnvilRenderer" << std::endl;
}

void AnvilRenderer::cleanup()
{
    // Wait for GPU
    vkDeviceWaitIdle(ptrAContext->anvilDevice);

    for (const AnvilFrame& anvilFrame : anvilFrames)
    {
        vkDestroySemaphore(ptrAContext->anvilDevice, anvilFrame.swapchainSemaphore, nullptr);
        vkDestroySemaphore(ptrAContext->anvilDevice, anvilFrame.renderSemaphore, nullptr);
        vkDestroyFence(ptrAContext->anvilDevice, anvilFrame.renderFence, nullptr);
        vkDestroyCommandPool(ptrAContext->anvilDevice, anvilFrame.cmdPool, nullptr);
    }
}

void AnvilRenderer::drawFrame(AnvilWindow& inWindow)
{
    AnvilFrame& frame = getCurrentFrame();

    // Wait for previous frame
    vkWaitForFences(ptrAContext->anvilDevice, 1, &frame.renderFence, VK_TRUE, UINT64_MAX);

    // Request image from swapchain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(ptrAContext->anvilDevice,
        ptrASwapchain->anvilSwapchain,
        UINT64_MAX,
        frame.swapchainSemaphore,
        VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // TODO: Implement OUT_OF_DATE_KHR handling
        // Recreate Swapchain
        ptrASwapchain->recreateSwapchain(inWindow.getFramebufferExtent());
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swapchain image.");
    }

    // Check if a previous frame is still using this swapchain image
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(ptrAContext->anvilDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = frame.renderFence;

    // Reset fences after the second wait and vkAcquireNextImageKHR
    vkResetFences(ptrAContext->anvilDevice, 1, &frame.renderFence);

    // Reset and begin command buffer
    VkCommandBuffer cmd = frame.cmdBuffer;
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Transition image here
    transitionImageLayout(cmd, ptrASwapchain->anvilImages[imageIndex],
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Begin Dynamic Rendering
    VkRenderingAttachmentInfo colorAttachmentInfo{};
    colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachmentInfo.imageView = ptrASwapchain->anvilImageViews[imageIndex];
    colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentInfo.clearValue.color = {{0.05f, 0.05f, 0.05f, 1.0f}};

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea = {{0, 0}, {ptrASwapchain->anvilExtent.width, ptrASwapchain->anvilExtent.height}};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachmentInfo;

    vkCmdBeginRendering(cmd, &renderInfo);

    // TODO: Pipeline binding

    // TODO: Hardcoded triangle for now

    vkCmdEndRendering(cmd);

    // Transition image to present layout
    transitionImageLayout(cmd, ptrASwapchain->anvilImages[imageIndex],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // End command buffer
    vkEndCommandBuffer(cmd);

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { frame.swapchainSemaphore };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = { frame.renderSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(ptrAContext->anvilGraphicsQueue, 1, &submitInfo, frame.renderFence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer.");
    }

    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchain = {ptrASwapchain->anvilSwapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(ptrAContext->anvilGraphicsQueue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        // Recreate Swapchain
        ptrASwapchain->recreateSwapchain(inWindow.getFramebufferExtent());
    }
    else if (presentResult != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swapchain image.");
    }

    // Increase frame number
    anvilFrameNumber++;
}

void AnvilRenderer::setupCommandBuffers()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = ptrAContext->anvilGraphicsQueueFamily;

    for (AnvilFrame& anvilFrame : anvilFrames)
    {
        if (vkCreateCommandPool(ptrAContext->anvilDevice, &poolInfo, nullptr, &anvilFrame.cmdPool) != VK_SUCCESS)
        {
            // TODO: Provide better error message
            throw std::runtime_error("Failed to create command pool.");
        }

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = anvilFrame.cmdPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(ptrAContext->anvilDevice, &allocInfo, &anvilFrame.cmdBuffer) != VK_SUCCESS)
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
        if (vkCreateSemaphore(ptrAContext->anvilDevice, &semaphoreInfo, nullptr, &anvilFrame.swapchainSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swapchain semaphore.");
        }
        if (vkCreateSemaphore(ptrAContext->anvilDevice, &semaphoreInfo, nullptr, &anvilFrame.renderSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render semaphore.");
        }
        if (vkCreateFence(ptrAContext->anvilDevice, &fenceInfo, nullptr, &anvilFrame.renderFence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render fence.");
        }
    }

    imagesInFlight.resize(ptrASwapchain->anvilImages.size(), VK_NULL_HANDLE);
}

AnvilFrame& AnvilRenderer::getCurrentFrame()
{
    return anvilFrames[anvilFrameNumber % FRAMES_IN_FLIGHT];
}

void AnvilRenderer::transitionImageLayout(VkCommandBuffer inCmd, VkImage inImage, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = inImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;
        sourceFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    
    vkCmdPipelineBarrier(inCmd, sourceFlags, destinationFlags, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "RenderContext.h"

#include "DebugNames.h"
#include "Logger.h"
#include "Trace.h"
#include "VulkanContext.h"
#include "VulkanResult.h"
#include "VulkanStrings.h"
#include "VulkanUtilities.h"
#include "Window.h"

void RenderContext::initializeRenderContext(VulkanContext* inContext, Swapchain* inSwapchain)
{
    SCOPE_CPU;
    LOG_DEBUG("Initializing RenderContext.");

    pContext = inContext;
    pSwapchain = inSwapchain;

    setupCommandBuffers();
    setupSyncStructures();

    pContext->immediateSubmit([this](VkCommandBuffer cmd)
    {
        SCOPE_CPU_NAME("TracyContext[immediateSubmit]");
        LOG_DEBUG("Creating Tracy Context.");

        tracyVkCtx = TracyVkContext(pContext->physicalDevice, pContext->device, pContext->graphicsQueue, cmd);
    });

    FATAL(tracyVkCtx != nullptr, "Tracy Context creation failed.");

    const float timestamp_period = pContext->physicalDeviceProperties.limits.timestampPeriod;
    gpuProfiler.initializeGPUProfiler(pContext, timestamp_period, FRAMES_IN_FLIGHT);
}

RenderContext::~RenderContext()
{
    if (pContext && pContext->device)
    {
        vkDeviceWaitIdle(pContext->device);

        if (tracyVkCtx)
        {
            TracyVkDestroy(tracyVkCtx);
        }

        for (const Frame& f : frames)
        {
            vkDestroySemaphore(pContext->device, f.imageAvailableSemaphore, nullptr);
            vkDestroyFence(pContext->device, f.frameDoneFence, nullptr);
            vkDestroyCommandPool(pContext->device, f.cmdPool, nullptr);
        }

        for (const VkSemaphore& semaphore : renderFinishedSemaphores)
        {
            vkDestroySemaphore(pContext->device, semaphore, nullptr);
        }
    }
}

void RenderContext::setupCommandBuffers()
{
    SCOPE_CPU;
    LOG_TRACE("Setting up Command Buffers");

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = pContext->graphicsQueueIndex;

    for (Index32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        Frame& f = frames[i];
        CHECK(vkCreateCommandPool(pContext->device, &pool_info, nullptr, &f.cmdPool));
        SET_DNAME_HERE(pContext->device, f.cmdPool, VK_OBJECT_TYPE_COMMAND_POOL, ("ComamndPool:" + std::to_string(i)).c_str());

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = f.cmdPool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;
        CHECK(vkAllocateCommandBuffers(pContext->device, &alloc_info, &f.cmdBuffer));
        SET_DNAME_HERE(pContext->device, f.cmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, ("ComamndBuffer:" + std::to_string(i)).c_str());
    }

    LOG_TRACE("Command Buffers setup finished.");
}

void RenderContext::setupSyncStructures()
{
    SCOPE_CPU;
    LOG_TRACE("Setting up Sync Structures");

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (Index32 i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        Frame& f = frames[i];

        CHECK(vkCreateSemaphore(pContext->device, &semaphore_info, nullptr, &f.imageAvailableSemaphore));
        SET_DNAME_HERE(pContext->device, f.imageAvailableSemaphore, VK_OBJECT_TYPE_SEMAPHORE, ("ImageAvailableSemaphore:" + std::to_string(i)).c_str());

        CHECK(vkCreateFence(pContext->device, &fence_info, nullptr, &f.frameDoneFence));
        SET_DNAME_HERE(pContext->device, f.frameDoneFence, VK_OBJECT_TYPE_FENCE, ("FrameDoneFence:" + std::to_string(i)).c_str());
    }

    // Create semaphores based on swapchain images count
    renderFinishedSemaphores.resize(pSwapchain->swapchainImages.size());
    for (Index32 i = 0; i < renderFinishedSemaphores.size(); ++i)
    {
        CHECK(vkCreateSemaphore(pContext->device, &semaphore_info, nullptr, &renderFinishedSemaphores[i]));
        SET_DNAME_HERE(pContext->device, renderFinishedSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE, ("RenderFinishedSemaphore:" + std::to_string(i)).c_str());
    }
}

VkCommandBuffer RenderContext::beginFrame(const Window& inWindow)
{
    SCOPE_CPU;

    if (recreateSwapchain)
    {
        vkDeviceWaitIdle(pContext->device);
        pSwapchain->recreateSwapchain(inWindow.getFramebufferExtent());
        recreateSwapchain = false;
    }

    const Frame& frame = getCurrentFrame();
    CHECK(vkWaitForFences(pContext->device, 1, &frame.frameDoneFence, VK_TRUE, UINT64_MAX));

    VkResult acquired_result = vkAcquireNextImageKHR(pContext->device,
        pSwapchain->anvilSwapchain,
        UINT64_MAX,
        frame.imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &imageIndex);

    if (acquired_result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        LOG_INFO("Recreate Swapchain. vkAcquireNextImageKHR returned: {}", vk_str(acquired_result));
        recreateSwapchain = true;
        return VK_NULL_HANDLE;
    }

    if (acquired_result != VK_SUCCESS && acquired_result != VK_SUBOPTIMAL_KHR)
    {
        LOG_ERROR("Failed to acquire next image: {}", vk_str(acquired_result));
    }

    CHECK(vkResetFences(pContext->device, 1, &frame.frameDoneFence));
    ENSURE(frameIndex < FRAMES_IN_FLIGHT, "Frame index should not be greater that the maximum frames in flight allowed.");
    ENSURE(imageIndex < pSwapchain->swapchainImages.size(), "Image index should not be grater than the total swapchain images available.");

    VkCommandBuffer cmd = frame.cmdBuffer;
    CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CHECK(vkBeginCommandBuffer(cmd, &begin_info));

    TracyVkCollect(tracyVkCtx, cmd);
    gpuProfiler.beginGPUProfilerFrame(cmd, frameIndex);

    return cmd;
}

void RenderContext::endFrame()
{
    SCOPE_CPU;

    const Frame& frame = getCurrentFrame();
    VkCommandBuffer cmd = frame.cmdBuffer;

    {
        SCOPE_GPU(tracyVkCtx, cmd, "EndFrame");

        VulkanUtils::TransitionImage(cmd, pSwapchain->swapchainImages[imageIndex], pSwapchain->swapchainImageLayouts[imageIndex], ImageLayout::Present);

        gpuProfiler.endGPUProfilerFrame(cmd, frameIndex);
    }

    CHECK(vkEndCommandBuffer(cmd));

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &frame.imageAvailableSemaphore;
    submit_info.pWaitDstStageMask = &wait_stage;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &renderFinishedSemaphores[imageIndex];
    CHECK(vkQueueSubmit(pContext->graphicsQueue, 1, &submit_info, frame.frameDoneFence));

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &renderFinishedSemaphores[imageIndex];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &pSwapchain->anvilSwapchain;
    present_info.pImageIndices = &imageIndex;

    VkResult present_result = vkQueuePresentKHR(pContext->graphicsQueue, &present_info);

    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
    {
        LOG_INFO("Recreate swapchain. vkQueuePresentKHR = {}.", vk_str(present_result));
        recreateSwapchain = true;
    }
    else if (present_result != VK_SUCCESS)
    {
        LOG_ERROR("Failed to present image: {}.", vk_str(present_result));
    }

    frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
    ENSURE(sizeof(frames) / sizeof(Frame) == FRAMES_IN_FLIGHT, "Number of frames prepared too large.");
    ENSURE(frameIndex < FRAMES_IN_FLIGHT, "Frame index should be less that max frames in fight.");
}


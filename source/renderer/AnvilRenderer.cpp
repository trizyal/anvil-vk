// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilRenderer.h"

#include <iostream>
#include <stdexcept>

#include "ShaderCompiler.h"
#include "UIRenderer.h"
#include "VulkanContext.h"
#include "Window.h"
#include "DebugNames.h"
#include "UIElements.h"
#include "VulkanResult.h"

void AnvilRenderer::initializeRenderer(VulkanContext* inAnvilContext, Swapchain* inAnvilSwapchain)
{
    std::cout << "Initializing AnvilRenderer" << std::endl;
    this->pContext = inAnvilContext;
    this->pSwapchain = inAnvilSwapchain;

    setupCommandBuffers();
    setupSyncStructures();

    const float timestamp_period = pContext->physicalDeviceProperties.limits.timestampPeriod;
    gpuProfiler.initializeGPUProfiler(pContext, timestamp_period, FRAMES_IN_FLIGHT);

    std::cout << "Finished Initializing AnvilRenderer" << std::endl;
}

AnvilRenderer::~AnvilRenderer()
{
    // Wait for GPU
    if (pContext && pContext->device)
    {
        vkDeviceWaitIdle(pContext->device);

        for (const AnvilFrame& anvil_frame : anvilFrames)
        {
            vkDestroySemaphore(pContext->device, anvil_frame.imageAvailableSemaphore, nullptr);
            vkDestroyFence(pContext->device, anvil_frame.frameDoneFence, nullptr);
            vkDestroyCommandPool(pContext->device, anvil_frame.cmdPool, nullptr);
        }

        // Clean up per-image semaphores
        for (const VkSemaphore& semaphore : renderFinishedSemaphores)
        {
            vkDestroySemaphore(pContext->device, semaphore, nullptr);
        }
    }
}

void AnvilRenderer::drawFrame(Window& inWindow, const RenderHooks& renderHooks)
{
    // Recreate swapchain maybe
    if (recreateSwapchain)
    {
        vkDeviceWaitIdle(pContext->device);
        pSwapchain->recreateSwapchain(inWindow.getFramebufferExtent());
        recreateSwapchain = false;
    }

    AnvilFrame& frame = getCurrentFrame();

    // Wait for previous frame
    VkResult fence_result = vkWaitForFences(pContext->device, 1, &frame.frameDoneFence, VK_TRUE, UINT64_MAX);
    if (fence_result != VK_SUCCESS)
    {
        std::ostringstream error_stream;
        error_stream << "Failed to Wait for frameDoneFence:" << std::endl;
        error_stream << "   Error: " << VulkanResult::ToString(fence_result) << std::endl;
        throw std::runtime_error(error_stream.str());
    }

    // Request image from swapchain
    uint32_t image_index = 0;
    VkResult acquired_result = vkAcquireNextImageKHR(pContext->device,
        pSwapchain->anvilSwapchain,
        UINT64_MAX,
        frame.imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &image_index);

    if (acquired_result == VK_ERROR_OUT_OF_DATE_KHR /*|| acquiredResult == VK_SUBOPTIMAL_KHR*/)
    {
        // Recreate Swapchain
        std::cout << "VK_ERROR_OUT_OF_DATE_KHR" << std::endl;
        recreateSwapchain = true;
        return;
    }

    if (acquired_result != VK_SUCCESS && acquired_result != VK_SUBOPTIMAL_KHR)
    {
        std::ostringstream error_stream;
        error_stream << "Failed to Acquire Next Image:" << std::endl;
        error_stream << "   Error: " << VulkanResult::ToString(acquired_result) << std::endl;
        throw std::runtime_error(error_stream.str());
    }

    // Reset fences after vkAcquireNextImageKHR
    fence_result = vkResetFences(pContext->device, 1, &frame.frameDoneFence);
    if (fence_result != VK_SUCCESS)
    {
        std::ostringstream error_stream;
        error_stream << "Failed to Reset frameDoneFence:" << std::endl;
        error_stream << "   Error: " << VulkanResult::ToString(fence_result) << std::endl;
        throw std::runtime_error(error_stream.str());
    }

    assert(anvilFrameIndex < FRAMES_IN_FLIGHT);
    assert(image_index < pSwapchain->swapchainImages.size());

    // Reset and begin command buffer
    VkCommandBuffer cmd = frame.cmdBuffer;
    vkResetCommandBuffer(cmd, 0);

    engineStats.resetFrameStats();
    auto cpu_start = std::chrono::high_resolution_clock::now();

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin_info);

    gpuProfiler.beginGPUProfilerFrame(cmd, anvilFrameIndex);

    if (renderHooks.onPreSwapchain)
    {
        // G-Buffer Geometry Pass
        renderHooks.onPreSwapchain(cmd, pSwapchain);
    }

    // Transition image here
    transitionImageLayout(cmd, pSwapchain->swapchainImages[image_index],
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // Transition Depth Image
    transitionImageLayout(cmd, pSwapchain->depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    // Begin Dynamic Rendering
    VkRenderingAttachmentInfo color_attachment_info{};
    color_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment_info.imageView = pSwapchain->swapchainImageViews[image_index];
    color_attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_info.clearValue.color = {{0.05f, 0.05f, 0.05f, 1.0f}};

    // 2. Define the Depth Attachment
    VkRenderingAttachmentInfo depth_attachment_info{.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depth_attachment_info.imageView = pSwapchain->depthImageView;
    depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment_info.clearValue.depthStencil = {1.0f, 0}; // 1.0 is the furthest depth

    VkRenderingInfo rendering_info{};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea = {{0, 0}, {pSwapchain->swapchainExtent.width, pSwapchain->swapchainExtent.height}};
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment_info;
    rendering_info.pDepthAttachment = &depth_attachment_info;

    vkCmdBeginRendering(cmd, &rendering_info);

    // --- EXECUTE PROJECT POLICY ---
    // Anvil has no idea what is being drawn here, it just executes the user's code.
    if (renderHooks.onSwapchain)
    {
        renderHooks.onSwapchain(cmd, pSwapchain);
    }

    engineStats.fps = 1000.f/engineStats.frameTime;
    UI::FrameStats(engineStats);
    UIRenderer::RecordUICommands(cmd);

    vkCmdEndRendering(cmd);

    // Transition image to present layout
    transitionImageLayout(cmd, pSwapchain->swapchainImages[image_index],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    gpuProfiler.endGPUProfilerFrame(cmd, anvilFrameIndex);

    // End command buffer
    vkEndCommandBuffer(cmd);

    auto cpu_end = std::chrono::high_resolution_clock::now();
    engineStats.cpuTime = std::chrono::duration<float, std::milli>(cpu_end - cpu_start).count();

    // Submit command buffer
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = { frame.imageAvailableSemaphore };
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    VkSemaphore signal_semaphores[] = { renderFinishedSemaphores[image_index] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    CHECK(vkQueueSubmit(pContext->graphicsQueue, 1, &submit_info, frame.frameDoneFence));

    // Present
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchain = {pSwapchain->anvilSwapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &image_index;

    VkResult present_result = vkQueuePresentKHR(pContext->graphicsQueue, &present_info);

    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
    {
        std::cout << "VK_ERROR_OUT_OF_DATE_KHR || VK_SUBOPTIMAL_KHR" << std::endl;
        recreateSwapchain = true;
    }
    else if (present_result != VK_SUCCESS)
    {
        std::ostringstream error_stream;
        error_stream << "Failed to Present Swapchain Image:" << std::endl;
        error_stream << "   Error: " << VulkanResult::ToString(present_result) << std::endl;
        throw std::runtime_error(error_stream.str());
    }

    engineStats.gpuTime = gpuProfiler.getGPUTime(anvilFrameIndex);

    anvilFrameIndex++;
    assert(sizeof(anvilFrames) / sizeof(AnvilFrame) == FRAMES_IN_FLIGHT);
    anvilFrameIndex %= FRAMES_IN_FLIGHT;
    assert(anvilFrameIndex < FRAMES_IN_FLIGHT);
}

void AnvilRenderer::setupCommandBuffers()
{
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = pContext->graphicsQueueIndex;

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        AnvilFrame& anvil_frame = anvilFrames[i];
        if (vkCreateCommandPool(pContext->device, &pool_info, nullptr, &anvil_frame.cmdPool) != VK_SUCCESS)
        {
            // TODO: Provide better error message
            throw std::runtime_error("Failed to create command pool.");
        }

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = anvil_frame.cmdPool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        CHECK(vkAllocateCommandBuffers(pContext->device, &alloc_info, &anvil_frame.cmdBuffer));

#if ANVIL_DEBUG
        // When function structure doesn't allow ANVIL_DEBUG_NAME, we can directly use the SetAutoName function
        std::string pool_name = "AnvilFrame[" + std::to_string(i) + "]_CommandPool";
        std::string cmd_name  = "AnvilFrame[" + std::to_string(i) + "]_CommandBuffer";

        // We can rely on the default std::source_location parameter here!
        SET_DNAME_HERE(pContext->device, reinterpret_cast<uint64_t>(anvil_frame.cmdPool),
                                VK_OBJECT_TYPE_COMMAND_POOL, pool_name.c_str());

        SET_DNAME_HERE(pContext->device, reinterpret_cast<uint64_t>(anvil_frame.cmdBuffer),
                                VK_OBJECT_TYPE_COMMAND_BUFFER, cmd_name.c_str());
#endif
    }
}

void AnvilRenderer::setupSyncStructures()
{
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        AnvilFrame& anvil_frame = anvilFrames[i];

        if (vkCreateSemaphore(pContext->device, &semaphore_info, nullptr, &anvil_frame.imageAvailableSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create imageAvailableSemaphore.");
        }
        std::string debug_name = "Frame[" + std::to_string(i) + "]_ImageAvailableSemaphore";
        SET_DNAME_HERE(pContext->device, anvil_frame.imageAvailableSemaphore, VK_OBJECT_TYPE_SEMAPHORE, debug_name.c_str());

        if (vkCreateFence(pContext->device, &fence_info, nullptr, &anvil_frame.frameDoneFence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create frameDoneFence.");
        }
        debug_name = "Frame[" + std::to_string(i) + "]_FrameDoneFence";
        SET_DNAME_HERE(pContext->device, anvil_frame.frameDoneFence, VK_OBJECT_TYPE_FENCE, debug_name.c_str());
    }

    // Create semaphores based on swapchain images count
    renderFinishedSemaphores.resize(pSwapchain->swapchainImages.size());
    for (uint32_t i = 0; i < renderFinishedSemaphores.size(); i++)
    {
        if (vkCreateSemaphore(pContext->device, &semaphore_info, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create renderFinishedSemaphore.");
        }
        std::string render_finished_name = "SwapchainImage[" + std::to_string(i) + "]_RenderFinishedSemaphore";
        SET_DNAME_HERE(pContext->device, renderFinishedSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE, render_finished_name.c_str());
    }
}

AnvilFrame& AnvilRenderer::getCurrentFrame()
{
    return anvilFrames[anvilFrameIndex % FRAMES_IN_FLIGHT];
}

void AnvilRenderer::transitionImageLayout(VkCommandBuffer inCmd, VkImage inImage, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier image_barrier{};
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.oldLayout = oldLayout;
    image_barrier.newLayout = newLayout;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = inImage;
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        image_barrier.srcAccessMask = 0;
        image_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        src_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        image_barrier.dstAccessMask = 0;
        src_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
    {
        image_barrier.srcAccessMask = 0;
        image_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        src_stage_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        image_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        image_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        src_stage_flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else
    {
        throw std::invalid_argument("Unsupported layout transition!");
    }
    
    vkCmdPipelineBarrier(inCmd, src_stage_flags, dst_stage_mask, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
}

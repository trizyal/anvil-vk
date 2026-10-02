// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Renderer.h"

#include <iostream>
#include <fstream>
#include <stdexcept>

#include "Camera.h"
#include "Console.h"
#include "CPUModel.h"
#include "DebugModes.h"
#include "ShaderCompiler.h"
#include "UIRenderer.h"
#include "VulkanContext.h"
#include "Window.h"
#include "DebugNames.h"
#include "GPUModel.h"
#include "Logger.h"
#include "PushConstants.h"
#include "ScreenLogger.h"
#include "UIElements.h"
#include "VulkanResult.h"
#include "Trace.h"
#include "VulkanStrings.h"

CVAR_INT("r.debugmode",
         "0: None"
         "1: Base Color"
         "2: Raw Normal Maps"
         "3: World Normal"
         "4: Metallic"
         "5: Roughness"
         "6: Depth",
         0
);

CVAR_BOOL("r.freezerendering", "Freezes the rendering state on the frame.", false);
CVAR_BOOL("r.frustumculling", "Enable frustum culling.", true);

void Renderer::initializeRenderer(VulkanContext* inAnvilContext, Swapchain* inAnvilSwapchain)
{
    SCOPE_CPU;
    LOG_TRACE("Initializing Renderer");

    this->pContext = inAnvilContext;
    this->pSwapchain = inAnvilSwapchain;

    setupCommandBuffers();
    setupSyncStructures();

    pContext->immediateSubmit([this]([[maybe_unused]]VkCommandBuffer cmd)
    {
        SCOPE_CPU_NAME("TracyContext(immediateSubmit)");
        LOG_INFO("Creating Tracy Context");
        tracyVkCtx = TracyVkContext(pContext->physicalDevice, pContext->device, pContext->graphicsQueue, cmd);
    });

    const float timestamp_period = pContext->physicalDeviceProperties.limits.timestampPeriod;
    gpuProfiler.initializeGPUProfiler(pContext, timestamp_period, FRAMES_IN_FLIGHT);

    engineCompiler.initializeShaderCompiler();
    engineCompiler.addSearchPath(SHADER_DIR);
    debugPass.initializeDebugPass(*pContext, engineCompiler, pSwapchain->swapchainFormat, pSwapchain->depthFormat);

    COMMAND("freezerendering",
        "Freezes the rendering state.",
        [](const std::vector<std::string>&)
        {
            bool current = Console::GetCVarBool("r.freezerendering");
            Console::SetCVarBool("r.freezerendering", !current);
            Console::Print(current ? "Rendering un-frozen." : "Rendering frozen.");
        }
    );

    COMMAND("vmastats",
        "Dumps VMA memory stats to vma_dumps.json.",
        [this](const std::vector<std::string>&)
        {
            char* stats_string;
            vmaBuildStatsString(pContext->allocator, &stats_string, VK_TRUE);

            // Write to file
            std::string dump_file_path = SAVED_DIR "/vma_dumps.json";
            std::ofstream dump_file;
            dump_file.open("vma_dumps.json");
            if (dump_file.is_open())
            {
                dump_file << stats_string;
                dump_file.close();
                Console::Print("Successfully dumped VMA stats to vma_dumps.json.");
                LOGUI("VMA Stats Dumped", AnvilColor::Blue);
            }
            else
            {
                Console::Print("Failed to open vma_dumps.json for writing.");
                LOGUI("Failed to dump VMA Stats", AnvilColor::Red);
            }

            vmaFreeStatsString(pContext->allocator, stats_string);
        }
    );

    LOG_TRACE("Finished initializing Renderer");
}

Renderer::~Renderer()
{
    // Wait for GPU
    if (pContext && pContext->device)
    {
        vkDeviceWaitIdle(pContext->device);

        if (tracyVkCtx)
        {
            TracyVkDestroy(tracyVkCtx);
        }

        debugPass.cleanupDebugPass();
        engineCompiler.shutdownShaderCompiler();

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

void Renderer::drawFrame(Window& inWindow, const RenderHooks& renderHooks)
{
    SCOPE_CPU;

    // Recreate swapchain maybe
    if (recreateSwapchain)
    {
        vkDeviceWaitIdle(pContext->device);
        pSwapchain->recreateSwapchain(inWindow.getFramebufferExtent());
        recreateSwapchain = false;
    }

    AnvilFrame& frame = getCurrentFrame();

    CHECK(vkWaitForFences(pContext->device, 1, &frame.frameDoneFence, VK_TRUE, UINT64_MAX));

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
        LOG_DEBUG("Recreate swapchain. vkAcquireNextImageKHR = VK_ERROR_OUT_OF_DATE_KHR.");
        recreateSwapchain = true;
        return;
    }

    if (acquired_result != VK_SUCCESS && acquired_result != VK_SUBOPTIMAL_KHR)
    {
        std::ostringstream error_stream;
        error_stream << "Failed to Acquire Next Image:" << std::endl;
        error_stream << "   Error: " << vk_str(acquired_result) << std::endl;
        LOG_FATAL("{}", error_stream.str());
    }

    // Reset fences after vkAcquireNextImageKHR
    CHECK(vkResetFences(pContext->device, 1, &frame.frameDoneFence));

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

    TracyVkCollect(tracyVkCtx, cmd);

    gpuProfiler.beginGPUProfilerFrame(cmd, anvilFrameIndex);

    {
        SCOPE_GPU(tracyVkCtx, cmd, "DrawFrame");
        if (renderHooks.onPreSwapchain)
        {
            SCOPE_GPU(tracyVkCtx, cmd, "Offscreen / Geometry Pass");
            // G-Buffer Geometry Pass
            renderHooks.onPreSwapchain(cmd, pSwapchain);
        }

        // Transition image here
        TransitionImageLayout(cmd, pSwapchain->swapchainImages[image_index],
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // Transition Depth Image
        TransitionImageLayout(cmd, pSwapchain->depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

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
    }

    // --- EXECUTE PROJECT POLICY ---
    // Anvil has no idea what is being drawn here, it just executes the user's code.
    if (renderHooks.onSwapchain)
    {
        SCOPE_GPU(tracyVkCtx, cmd, "Main Swapchain Pass");
        renderHooks.onSwapchain(cmd, pSwapchain);
    }

    {
        SCOPE_GPU(tracyVkCtx, cmd, "UI RenderPass");
        engineStats.fps = 1000.f/engineStats.frameTime;
        UI::FrameStats(engineStats);
        UIRenderer::RecordUICommands(cmd);
    }

    vkCmdEndRendering(cmd);

    // Transition image to present layout
    {
        SCOPE_GPU(tracyVkCtx, cmd, "Transition to Present");
        TransitionImageLayout(cmd, pSwapchain->swapchainImages[image_index],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

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
        LOG_DEBUG("Recreate swapchain. vkQueuePresentKHR = VK_ERROR_OUT_OF_DATE_KHR || VK_SUBOPTIMAL_KHR.");
        recreateSwapchain = true;
    }
    else if (present_result != VK_SUCCESS)
    {
        std::ostringstream error_stream;
        error_stream << "Failed to Present Swapchain Image:" << std::endl;
        error_stream << "   Error: " << vk_str(present_result) << std::endl;
        LOG_FATAL("{}", error_stream.str());
    }

    engineStats.gpuTime = gpuProfiler.getGPUTime(anvilFrameIndex);

    anvilFrameIndex++;
    assert(sizeof(anvilFrames) / sizeof(AnvilFrame) == FRAMES_IN_FLIGHT);
    anvilFrameIndex %= FRAMES_IN_FLIGHT;
    assert(anvilFrameIndex < FRAMES_IN_FLIGHT);

    SCOPE_FRAME;
}

void Renderer::drawModel(VkCommandBuffer inCmd, const GPUModel& model, const Camera& camera, VkPipeline userPipeline, VkPipelineLayout userLayout, VkDescriptorSet userSet0, bool isGBufferPass) const
{
    SCOPE_CPU;
    SCOPE_GPU(tracyVkCtx, inCmd, "Draw Model");

    uint32_t debug_mode = static_cast<uint32_t>(Console::GetCVarInt("r.debugmode"));
    bool is_forward_debug = DebugPass::isForwardMode(debug_mode);
    bool is_debug = static_cast<DebugMode>(debug_mode) > DebugMode::None;
    bool is_frozen = Console::GetCVarBool("r.freezerendering");
    bool is_culling = Console::GetCVarBool("r.frustumculling");

    // Skip G-Buffer geometry generation completely if we are doing a Deferred Debug Pass
    if (isGBufferPass)
    {
        if (is_forward_debug)
        {
            return;
        }
    }

    // Only apply the debug pipeline if we are rendering forward directly to the Swapchain
    bool use_debug_pipeline = !isGBufferPass && is_debug;

    // Only Forward Debug Passes and User Pass is left
    VkPipeline active_pipeline = use_debug_pipeline ? debugPass.getForwardPipeline(debug_mode).pipeline : userPipeline;
    VkPipelineLayout active_layout = use_debug_pipeline ? debugPass.getForwardLayout() : userLayout;

    const float aspect = static_cast<float>(pSwapchain->swapchainExtent.width) /
                         static_cast<float>(pSwapchain->swapchainExtent.height);
    const glm::mat4 projection = camera.getProjectionMatrix(aspect);
    const glm::mat4 view = camera.getViewMatrix();
    const glm::mat4 view_projection = projection * view;

    static glm::mat4 frozen_vp = view_projection;
    static bool was_frozen = false;
    if (is_frozen && !was_frozen)
    {
        frozen_vp = view_projection;
    }
    was_frozen = is_frozen;

    Frustum camera_frustum{};
    camera_frustum.extractPlanes(is_frozen ? frozen_vp : view_projection);

    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, active_pipeline);

    VkDeviceSize offset = 0;
    for (Index32 i = 0 ; i < model.drawItems.size() ; ++i)
    {
        const GPUModelDrawItem& draw_item = model.drawItems[i];
        SCOPE_GPU(tracyVkCtx, inCmd, "Draw Item");
        TracyVkZoneTransient(tracyVkCtx, boo,inCmd, DebugPass::GetDebugModeName(static_cast<DebugMode>(debug_mode)), true);


        if (draw_item.gpuMeshIndex >= model.gpuMeshes.size())
        {
            continue;
        }

        if (is_culling)
        {
            SCOPE_CPU_NAME("Frustum Culling")
            // Fast AABB World Transform & Frustum Check
            glm::vec3 center = draw_item.localBounds.getCenter();
            glm::vec3 extents = draw_item.localBounds.getExtents();
            glm::vec3 worldCenter = glm::vec3(draw_item.worldMatrix * glm::vec4(center, 1.0f));
            glm::mat3 absModel = glm::mat3(
                glm::abs(draw_item.worldMatrix[0]),
                glm::abs(draw_item.worldMatrix[1]),
                glm::abs(draw_item.worldMatrix[2])
            );
            glm::vec3 worldExtents = absModel * extents;

            AABB worldAABB{ .min = worldCenter - worldExtents, .max = worldCenter + worldExtents };

            if (!camera_frustum.contains(worldAABB))
            {
                continue; // culled
            }
        }

        std::vector<VkDescriptorSet> sets;
        uint32_t first_set = 1;

        if (!use_debug_pipeline && userSet0 != VK_NULL_HANDLE)
        {
            first_set = 0;
            sets.push_back(userSet0);
        }

        // Safely fallback if a primitive is missing a material
        VkDescriptorSet matSet = VK_NULL_HANDLE;
        if (draw_item.gpuMaterialIndex >= 0 && draw_item.gpuMaterialIndex < static_cast<int>(model.gpuMaterials.size()))
        {
            matSet = model.gpuMaterials[draw_item.gpuMaterialIndex].instance.descriptorSet;
        }
        else if (!model.gpuMaterials.empty())
        {
            matSet = model.gpuMaterials[0].instance.descriptorSet;
        }

        // Push Set 1 and Set 2
        if (model.modelSet.descriptorSet != VK_NULL_HANDLE)
        {
            sets.push_back(model.modelSet.descriptorSet);
        }
        if (matSet != VK_NULL_HANDLE)
        {
            sets.push_back(matSet);
        }

        vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, active_layout, first_set, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

        PushConstants constants{};
        constants.viewProjection = view_projection;
        constants.cameraPosition = glm::vec4(camera.position, 1.0f);
        constants.objectIndex = i; // Map to SSBO index
        constants.debugMode = static_cast<DebugMode>(debug_mode);
        vkCmdPushConstants(inCmd, active_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &constants);

        const GPUMesh& mesh = model.gpuMeshes[draw_item.gpuMeshIndex];
        vkCmdBindVertexBuffers(inCmd, 0, 1, &mesh.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(inCmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(inCmd, mesh.indexCount, 1, 0, 0, 0);

        engineStats.drawCalls++;
        engineStats.primitiveCount += (mesh.indexCount / 3);
    }
}

void Renderer::drawDeferredLighting(VkCommandBuffer inCmd, GBuffer& gBuffer, const Camera& camera, VkPipeline userPipeline, VkPipelineLayout userLayout, VkDescriptorSet userSet0)
{
    SCOPE_CPU;
    uint32_t debug_mode = static_cast<uint32_t>(Console::GetCVarInt("r.debugmode"));

    if (static_cast<DebugMode>(debug_mode) == DebugMode::None)
    {
        SCOPE_GPU(tracyVkCtx, inCmd, "Deferred Lighting");
        vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, userPipeline);
        vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, userLayout, 0, 1, &userSet0, 0, nullptr);

        PushConstants pc{};
        pc.cameraPosition = glm::vec4(camera.position, 1.0f);
        pc.debugMode = DebugMode::None;
        vkCmdPushConstants(inCmd, userLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);
        
        vkCmdDraw(inCmd, 3, 1, 0, 0);
    }
    else if (DebugPass::isDeferredMode(debug_mode))
    {
        SCOPE_GPU(tracyVkCtx, inCmd, "Deferred Debug");
        TracyVkZoneTransient(tracyVkCtx, boo,inCmd, DebugPass::GetDebugModeName(static_cast<DebugMode>(debug_mode)), true);
        debugPass.drawDeferredResolve(inCmd, gBuffer, static_cast<DebugMode>(debug_mode), glm::vec4(camera.position, 1.0f));
    }
}


void Renderer::setupCommandBuffers()
{
    SCOPE_CPU;
    LOG_TRACE("Setting up Command Buffers");

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = pContext->graphicsQueueIndex;

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        AnvilFrame& anvil_frame = anvilFrames[i];
        CHECK(vkCreateCommandPool(pContext->device, &pool_info, nullptr, &anvil_frame.cmdPool));

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = anvil_frame.cmdPool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        CHECK(vkAllocateCommandBuffers(pContext->device, &alloc_info, &anvil_frame.cmdBuffer));

#if ANVIL_DEBUG
        LOG_DEBUG("Debug names for Command Buffer data set.");
        // When function structure doesn't allow ANVIL_DEBUG_NAME, we can directly use the SetAutoName function
        std::string pool_name = "AnvilFrame[" + std::to_string(i) + "]_CommandPool";
        std::string cmd_name  = "AnvilFrame[" + std::to_string(i) + "]_CommandBuffer";

        // We can rely on the default std::source_location parameter here!
        SET_DNAME_HERE(pContext->device, anvil_frame.cmdPool, VK_OBJECT_TYPE_COMMAND_POOL, pool_name.c_str());
        SET_DNAME_HERE(pContext->device, anvil_frame.cmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, cmd_name.c_str());
#endif
    }

    LOG_TRACE("Command Buffers setup finished.");
}

void Renderer::setupSyncStructures()
{
    SCOPE_CPU;
    LOG_TRACE("Setting up Sync Structures");

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        AnvilFrame& anvil_frame = anvilFrames[i];

        CHECK(vkCreateSemaphore(pContext->device, &semaphore_info, nullptr, &anvil_frame.imageAvailableSemaphore));
        SET_DNAME_HERE(pContext->device, anvil_frame.imageAvailableSemaphore, VK_OBJECT_TYPE_SEMAPHORE,
            ("Frame[" + std::to_string(i) + "]_ImageAvailableSemaphore").c_str());

        CHECK(vkCreateFence(pContext->device, &fence_info, nullptr, &anvil_frame.frameDoneFence));
        SET_DNAME_HERE(pContext->device, anvil_frame.frameDoneFence, VK_OBJECT_TYPE_FENCE,
            ("Frame[" + std::to_string(i) + "]_FrameDoneFence").c_str());
    }

    // Create semaphores based on swapchain images count
    renderFinishedSemaphores.resize(pSwapchain->swapchainImages.size());
    for (uint32_t i = 0; i < renderFinishedSemaphores.size(); i++)
    {
        CHECK(vkCreateSemaphore(pContext->device, &semaphore_info, nullptr, &renderFinishedSemaphores[i]));
        SET_DNAME_HERE(pContext->device, renderFinishedSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE,
            ("SwapchainImage[" + std::to_string(i) + "]_RenderFinishedSemaphore").c_str());
    }
}

AnvilFrame& Renderer::getCurrentFrame()
{
    return anvilFrames[anvilFrameIndex % FRAMES_IN_FLIGHT];
}

void Renderer::TransitionImageLayout(VkCommandBuffer inCmd, VkImage inImage, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    SCOPE_CPU;

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
        LOG_ERROR("Unsupported layout transition!");
        ENSURE(false, "Unsupported layout transition");
    }
    
    vkCmdPipelineBarrier(inCmd, src_stage_flags, dst_stage_mask, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
}

void Renderer::SetViewportScissor(VkCommandBuffer inCmd, const Swapchain& inSwapchain)
{
    SCOPE_CPU;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(inSwapchain.swapchainExtent.width);
    viewport.height = static_cast<float>(inSwapchain.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(inCmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = inSwapchain.swapchainExtent;
    vkCmdSetScissor(inCmd, 0, 1, &scissor);
}

bool Renderer::reloadDebugShaders(std::string* outError)
{
    SCOPE_CPU;
    LOG_DEBUG("Reloading Debug Shaders.");

    // Force Slang to drop its module cache and read from disk again
    engineCompiler.resetSession();

    // 1. Create a temporary pass and attempt to initialize it
    DebugPass tempPass;
    bool bSuccess = tempPass.initializeDebugPass(*pContext, engineCompiler, pSwapchain->swapchainFormat, pSwapchain->depthFormat, outError);

    if (bSuccess)
    {
        // 2a. Compilation Succeeded!
        // Safely tear down the old pipelines first.
        debugPass.cleanupDebugPass();

        // Transfer ownership of the new Vulkan handles to the active debugPass.
        debugPass = std::move(tempPass);

        // Clear cached G-Buffer view so the new deferred descriptor set knows to rebind it.
        debugPass.cachedGBufferView = VK_NULL_HANDLE;
    }
    else
    {
        // 2b. Compilation Failed!
        // Clean up whatever partially compiled in the temporary pass.
        // The active debugPass remains completely untouched, preventing the VK_NULL_HANDLE crash.
        tempPass.cleanupDebugPass();
    }

    return bSuccess;
}

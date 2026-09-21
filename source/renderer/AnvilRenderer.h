// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_RENDERER_H
#define ANVIL_VK_RENDERER_H

/**
 * @file AnvilRenderer.h
 * @brief Core frame synchronization, command recording, and draw-loop orchestration.
 */

#include <functional>

#include "DebugPass.h"
#include "FrameStats.h"
#include "GPUProfiler.h"
#include "ShaderCompiler.h"
#include "Swapchain.h"

class Camera;
class GPUModel;
class Window;

/**
 * @brief Injection points for project-specific rendering logic during the frame lifecycle.
 */
struct RenderHooks
{
    /**
     * Executed BEFORE the swapchain rendering block begins.
     * Perfect for off-screen passes: Shadow mapping, G-Buffer generation, Compute shaders.
     */
    std::function<void(VkCommandBuffer, Swapchain*)> onPreSwapchain = nullptr;

    /**
     * Executed INSIDE the swapchain rendering block.
     * Used for drawing final geometry (Forward) or compositing lighting (Deferred), right before the UI renders.
     */
    std::function<void(VkCommandBuffer, Swapchain*)> onSwapchain = nullptr;
};

/**
 * @brief Per-frame GPU resources required for flight synchronized rendering.
 */
struct AnvilFrame
{
    /** Pool allocated specifically for this frame's command buffer. */
    VkCommandPool cmdPool = VK_NULL_HANDLE;

    /** Primary command buffer for recording draw commands. */
    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;

    /** Signaled when the swapchain image is ready to render to. */
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;

    /** CPU waits on this for the GPU to finish rendering the frame. */
    VkFence frameDoneFence = VK_NULL_HANDLE;
};

/**
 * @brief Number of concurrent frames the CPU can submit ahead of the GPU.
 */
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

/**
 * @brief Orchestrates the Vulkan draw loop, sync primitives, and frame timing.
 *
 * Manages multi-buffered frames in flight, handles swapchain recreation on resize,
 * and provides a callback interface for recording commands into the active buffer.
 *
 * @note This class is non-copyable and non-movable. May need to change that.
 */
class AnvilRenderer
{
public:
    AnvilRenderer() = default;

    /**
     * @brief Waits for the GPU to idle and destroys all per-frame Vulkan resources.
     */
    ~AnvilRenderer();

    /** Copy construction is disabled (Prevents double-freeing Vulkan sync objects & pools) */
    AnvilRenderer(const AnvilRenderer&) = delete;

    /** Copy assignment is disabled */
    AnvilRenderer& operator=(const AnvilRenderer&) = delete;

    /** Move construction is disabled */
    AnvilRenderer(AnvilRenderer&&) = delete;

    /** Move assignment is disabled */
    AnvilRenderer& operator=(AnvilRenderer&&) = delete;

private:
    VulkanContext* pContext = nullptr;
    Swapchain* pSwapchain = nullptr;

    /** Array of frame-sync structures for flighted rendering. */
    AnvilFrame anvilFrames[FRAMES_IN_FLIGHT];

    /** The current frame index (modulo FRAMES_IN_FLIGHT). */
    uint32_t anvilFrameIndex = 0;

    /** Signaled when rendering is finished, and image ready to present. */
    std::vector<VkSemaphore> renderFinishedSemaphores;

    /** Flag triggered when a window resize requires the swapchain to be rebuilt. */
    bool recreateSwapchain = false;

    GPUProfiler gpuProfiler;
    ShaderCompiler engineCompiler;
    DebugPass debugPass;

public:
    /** Global tracking of engine performance metrics (FPS, GPU/CPU time). */
    inline static FrameStats engineStats;

    /**
     * @brief Initializes frame resources, command pools, and synchronization primitives.
     *
     * @param inAnvilContext Pointer to the initialized Anvil Vulkan context.
     * @param inAnvilSwapchain Pointer to the active swapchain to render into.
     *
     * @throws std::runtime_error If command pools, buffers, or sync objects fail to create.
     */
    void initializeRenderer(VulkanContext* inAnvilContext, Swapchain* inAnvilSwapchain);

    /**
     * @brief Prepare a frame for rendering, executes the draw callback, and presents.
     *
     * Handles CPU-GPU synchronization, acquiring a swapchain image, executing user-provided
     * rendering commands, and submitting the result to the presentation queue.
     * Flags the swapchain for recreation if window resizing is detected.
     *
     * @param inWindow Reference to the Anvil Window to check for the resized or minimized state from GLFW.
     * @param renderHooks A lambda or function invoked with the active command buffer and swapchain.
     *
     * @note drawCallback is triggered after BeginRendering is called and before the UI renders.
     */
    void drawFrame(Window& inWindow, const RenderHooks& renderHooks);

    /**
     * @brief Handles frustum culling, culling freezes, and renders geometry.
     * Overrides rendering with forward debug shaders if necessary.
     *
     * @param inCmd Active Vulkan command buffer to record draw commands into.
     * @param model The GPU model containing the meshes, materials, and draw items to render.
     * @param camera The active camera used for frustum culling and view-projection matrices.
     * @param userPipeline The default graphics pipeline to use when not in a debug rendering mode.
     * @param userLayout The pipeline layout associated with the userPipeline.
     * @param userSet0 Optional user-provided descriptor set (Set 0) to bind alongside the model's internal sets.
     * @param isGBufferPass Flag indicating if this draw call is targeting the G-Buffer, which bypasses forward debug overrides.
     */
    void drawModel(VkCommandBuffer inCmd, const GPUModel& model, const Camera& camera, VkPipeline userPipeline,
        VkPipelineLayout userLayout, VkDescriptorSet userSet0, bool isGBufferPass = false) const;

    /**
     * @brief Resolves the G-Buffer lighting or injects deferred debug views.
     *
     * @param inCmd Active Vulkan command buffer to record the fullscreen resolve draw into.
     * @param gBuffer The populated G-Buffer containing geometry attachments (Albedo, Normals, PBR, Depth).
     * @param camera The active camera, used for position reconstruction in deferred debug views.
     * @param userPipeline The standard deferred lighting pipeline to use when not in a debug mode.
     * @param userLayout The pipeline layout associated with the userPipeline.
     * @param userSet0 The user-provided descriptor set containing the bound G-Buffer textures and lighting data.
     */
    void drawDeferredLighting(VkCommandBuffer inCmd, GBuffer& gBuffer, const Camera& camera, VkPipeline userPipeline,
        VkPipelineLayout userLayout, VkDescriptorSet userSet0);

    /**
     * @brief Helper to insert a Vulkan image memory barrier for layout transitions.
     *
     * @param inCmd Active command buffer to record the barrier into.
     * @param inImage The Vulkan image to transition.
     * @param oldLayout The current layout of the image.
     * @param newLayout The desired layout of the image.
     */
    static void TransitionImageLayout(VkCommandBuffer inCmd, VkImage inImage,
                                      VkImageLayout oldLayout, VkImageLayout newLayout);

    /**
     * @brief Helper to dynamically set the viewport and scissor rect to match the swapchain.
     *
     * @param inCmd Active command buffer.
     * @param inSwapchain The swapchain to pull the extent from.
     */
    static void SetViewportScissor(VkCommandBuffer inCmd, const Swapchain& inSwapchain);

    /**
     * @brief Recompiles and reloads the engine debug shaders at runtime.
     *
     * @param outError String to store compilation errors if the reload fails.
     * @return True if the shaders successfully compiled and reloaded, false otherwise.
     */
    bool reloadDebugShaders(std::string* outError);

private:
    /**
     * @brief Retrieves the frame sync structure for the current flight index.
     * @return Reference to the active AnvilFrame.
     */
    AnvilFrame& getCurrentFrame();

    /**
     * @brief Allocates command pools and buffers for all frames in flight.
     */
    void setupCommandBuffers();

    /**
     * @brief Creates semaphores and fences for CPU/GPU and Queue synchronization.
     */
    void setupSyncStructures();
};

#endif //ANVIL_VK_RENDERER_H

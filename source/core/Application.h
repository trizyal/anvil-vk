// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file Application.h
 * @brief High-level application lifecycle, window management, and main loop orchestration.
 */

#include <functional>
#include <memory>
#include <string>

#include <volk.h>

#include "DebugPass.h"
#include "FrameStats.h"
#include "RenderContext.h"
#include "ShaderCompiler.h"
#include "Swapchain.h"
#include "UIRenderer.h"
#include "VulkanContext.h"
#include "Window.h"

/**
 * @brief Configuration settings for initializing an Application instance.
 */
struct CreateInfo
{
    uint32_t width = 1280;              /**< Initial window viewport height in pixels. */
    uint32_t height = 720;              /**< Initial window viewport height in pixels. */
    std::string title = "Application";  /**< Window title bar text. */
};

/**
 * @brief Injection point for the application to build and execute its Render Graph.
 */
struct RenderHooks
{
    /** Invoked every frame with a ready-to-record command buffer. */
    std::function<void(VkCommandBuffer)> onRecordFrame = nullptr;

    /** Invoked every frame to submit UI commands. */
    std::function<void()> onDrawUI = nullptr;
};

/**
 * @brief Primary engine host that manages the OS window, vulkan context, and run loop.
 *
 * Serves as the central entry point for applications. Handles initialization
 * and cleanup order of the core Vulkan subsystems, windowing events, UI overlays,
 * and dynamic shader hot-reloading.
 *
 * @note This class is non-copyable and non-movable.
 */
class Application
{
public:
    Application() = default;
    ~Application() = default;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

private:
    bool applicationInitialized = false;

    std::unique_ptr<Window> window;
    VulkanContext context;
    Swapchain swapchain;

    // Rendering Subsystems
    RenderContext renderContext;
    ShaderCompiler shaderCompiler;
    DebugPass debugPass;
    UIRenderer uiRenderer;

    // Shader Reload Setup
    /** Queue of callbacks to execute when a shader hot-reload is triggered. */
    std::vector<std::function<bool(std::string*)>> shaderReloadQueue;
    /** Stores the latest output log from a failed shader compilation. */
    std::string activeShaderErrorLog;

    // UI Flags
    /** Flag indicating if the shader compilation error modal is currently active. */
    bool bShaderErrorModalOpen = false;
    /**
     * Tracks whether the developer console is currently rendering.
     * (0=Hidden, 1=Mini, 2=Full)
     */
    int consoleState = 0;

public:
    /** Global tracking of engine performance metrics (FPS, GPU/CPU time). */
    inline static FrameStats engineStats;

    /**
     * @brief Bootstraps the application window, input capturing, tools and all core Vulkan subsystems.
     * @param info Optional window and startup configuration struct.
     */
    void initialize(const CreateInfo& info = {});

    /**
     * @brief Starts the main application event loop and provides the renderer with the draw callback.
     * @param renderHooks Struct containing record frame callbacks.
     */
    void run(const RenderHooks& renderHooks);

    /**
     * @brief Shuts down the engine and safely destroys all resources.
     */
    void shutdown();

    /**
     * @brief Queues a callback function to be executed when a shader reload event occurs.
     * @param shaderCallback Callback returning bool (true = success) and filling error output string.
     */
    void addShaderReloadCallback(const std::function<bool(std::string*)>& shaderCallback)
    {
        shaderReloadQueue.push_back(shaderCallback);
    }

    /**
     * @brief Retrieves a reference to the active application window.
     * @return Reference to the Window instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]]
    Window& getWindow() const
    {
        return *window;
    }

    /**
     * @brief Retrieves the core context (instance, device, memory allocator).
     * @return Reference to the VulkanContext instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]]
    VulkanContext& getContext()
    {
        return context;
    }

    /**
     * @brief Retrieves the active swapchain.
     * @return Reference to the Swapchain instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]]
    Swapchain& getSwapchain()
    {
        return swapchain;
    }

    /**
     * @brief Retrieves the RenderContext(command pool, command buffer, sync structures).
     * @return Reference to the RenderContext instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]]
    RenderContext& getRenderContext()
    {
        return renderContext;
    }

    /**
     * @brief Retrieves the Shader Compiler.
     * @return Reference to the ShaderCompiler instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]]
    ShaderCompiler& getShaderCompiler()
    {
        return shaderCompiler;
    }

    /**
     * @brief Retrieves the Debug Pass.
     * @return Reference to the DebugPass instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]]
    DebugPass& getDebugPass()
    {
        return debugPass;
    }

private:
    /**
     * @brief Halts the GPU and triggers execution of all queued shader reload callbacks.
     *
     * If compilation fails, the error modal is opened and the GPU remains paused until
     * the user resolves the error or aborts.
     */
    void triggerShaderHotReload();

    /**
     * @brief Recompiles and reloads the engine debug shaders at runtime.
     *
     * @param outError String to store compilation errors if the reload fails.
     * @return True if the shaders successfully compiled and reloaded, false otherwise.
     */
    bool reloadDebugShaders(std::string* outError);
};

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_ANVIL_H
#define ANVIL_VK_ANVIL_H

/**
 * @file Anvil.h
 * @brief High-level application lifecycle, window management, and main loop orchestration.
 */

#include <memory>
#include <string>
#include <functional>

#include "Window.h"
#include "VulkanContext.h"
#include "Swapchain.h"
#include "AnvilRenderer.h"
#include "UIRenderer.h"

/**
 * @brief Configuration settings for initializing an AnvilApplication instance.
 */
struct AnvilCreateInfo
{
    uint32_t width = 1280;          /**< Initial window viewport height in pixels. */
    uint32_t height = 720;          /**< Initial window viewport height in pixels. */
    std::string title = "Anvil";    /**< Window title bar text. */
};

/**
 * @brief Primary engine host that manages the OS window, vulkan context, and run loop.
 *
 * Serves as the central entry point for Anvil applications. Handles initialization
 * and cleanup order of the core Vulkan subsystems, windowing events, UI overlays,
 * and dynamic shader hot-reloading.
 *
 * @note This class is non-copyable and non-movable due to underlying GLFW.
 */
class Anvil
{
public:
    Anvil()= default;
    ~Anvil() = default;

    /** Copy construction is disabled */
    Anvil(const Anvil&) = delete;

    /** Copy assignment is disabled */
    Anvil& operator=(const Anvil&) = delete;

    /** Move construction is disabled */
    Anvil(Anvil&&) = delete;

    /** Move assignment is disabled */
    Anvil& operator=(Anvil&&) = delete;

private:
    std::unique_ptr<Window> window;
    VulkanContext context;
    Swapchain swapchain;
    AnvilRenderer renderer;
    UIRenderer uiRenderer;

    /** Tracks whether the engine has been successfully bootstrapped. */
    bool initialized = false;

    /** Queue of callbacks to execute when a shader hot-reload is triggered. */
    std::vector<std::function<bool(std::string*)>> shaderReloadQueue;

    /** Flag indicating if the shader compilation error modal is currently active. */
    bool bShaderErrorModalOpen = false;

    /** Stores the latest output log from a failed shader compilation. */
    std::string activeShaderErrorLog;

    /**
     * Tracks whether the developer console is currently rendering.
     * (0=Hidden, 1=Mini, 2=Full)
     */
    int consoleState = 0;

public:
    /**
     * @brief Bootstraps the application window, input capturing, and all core Vulkan subsystems.
     *
     * Initializes GLFW, creates the Vulkan instance, device, memory allocator and debug utils,
     * sets up the swapchain, and initializes the renderer.
     * @param inCreateInfo Optional window and startup configuration struct.
     * @throws std::runtime_error If GLFW or any core Vulkan subsystems fail to initialize.
     */
    void initializeAnvil(const AnvilCreateInfo& inCreateInfo = {});

    /**
     * @brief Starts the main application event loop and provides the renderer with the draw callback.
     *
     * Runs continuously until the window is closed or an exit signal is received.
     * Automatically polls OS events, processes any queued shader reloads, and invokes
     * the provided render callback every frame.
     * @param renderHooks Struct containing optional pre-pass and main-pass callbacks.
     * @throws std::runtime_error If the AnvilApplication is uninitialized or `drawFrame` throws.
     * @attention Shader reloading happening here is not ideal.
     */
    void runAnvil(const RenderHooks& renderHooks);

    /**
     * @brief Shuts down the engine and safely destroys all Vulkan and window resources.
     *
     * Waits for the GPU device to idle before releasing handles.
     */
    void shutdownAnvil();

    /**
     * @brief Queues a callback function to be executed when a shader reload event occurs.
     * @param shaderCallback Callback returning bool (true = success) and filling error output string.
     */
    void addShaderReloadCallback(const std::function<bool(std::string*)>& shaderCallback);

    /**
     * @brief Retrieves a reference to the active application window.
     * @return Reference to the Window instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]]
    Window& getWindow() const;

    /**
     * @brief Retrieves the core Vulkan context (instance, device, memory allocator).
     * @return Reference to the VulkanContext instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]]
    VulkanContext& getContext();

    /**
     * @brief Retrieves the active Vulkan swapchain.
     * @return Reference to the Swapchain instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]]
    Swapchain& getSwapchain();

    /**
     * @brief Retrieves the main renderer responsible for command buffer orchestration.
     * @return Reference to the AnvilRenderer instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]]
    AnvilRenderer& getRenderer();

private:
    /**
     * @brief Halts the GPU and triggers execution of all queued shader reload callbacks.
     *
     * If compilation fails, the error modal is opened and the GPU remains paused until
     * the user resolves the error or aborts.
     */
    void triggerShaderHotReload();
};

#endif //ANVIL_VK_ANVIL_H

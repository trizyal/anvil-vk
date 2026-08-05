// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_APPLICATION_H
#define ANVIL_VK_APPLICATION_H

/**
 * @file AnvilApplication.h
 * @brief High-level application lifecycle, window management, and main loop orchestration.
 */

#include <memory>
#include <string>
#include <functional>

#include "AnvilWindow.h"
#include "AnvilVulkanContext.h"
#include "AnvilSwapchain.h"
#include "AnvilRenderer.h"
#include "AnvilUIRenderer.h"

/**
 * @brief Configuration settings for initializing an AnvilApplication instance.
 */
struct AnvilApplicationCreateInfo
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
class AnvilApplication
{
public:
    AnvilApplication()= default;
    ~AnvilApplication() = default;

    /** Copy construction is disabled */
    AnvilApplication(const AnvilApplication&) = delete;

    /** Copy assignment is disabled */
    AnvilApplication& operator=(const AnvilApplication&) = delete;

    /** Move construction is disabled */
    AnvilApplication(AnvilApplication&&) = delete;

    /** Move assignment is disabled */
    AnvilApplication& operator=(AnvilApplication&&) = delete;

private:
    std::unique_ptr<AnvilWindow> anvilWindow;
    AnvilVulkanContext anvilContext;
    AnvilSwapchain anvilSwapchain;
    AnvilRenderer anvilRenderer;
    AnvilUIRenderer anvilUIRenderer;
    bool anvilInitialized = false;

    std::vector<std::function<void()>> shaderReloadQueue;

public:
    /**
     * @brief Bootstraps the application window, input capturing, and all core Vulkan subsystems.
     *
     * Initializes GLFW, creates the Vulkan instance, device, memory allocator and debug utils,
     * sets up the swapchain, and initializes the renderer.
     * @param inCreateInfo Optional window and startup configuration struct.
     * @throws std::runtime_error If GLFW or any core Vulkan subsystems fail to initialize.
     */
    void initializeAnvil(const AnvilApplicationCreateInfo& inCreateInfo = {});

    /**
     * @brief Starts the main application event loop and provides the renderer with the draw callback.
     *
     * Runs continuously until the window is closed or an exit signal is received.
     * Automatically polls OS events, processes any queued shader reloads, and invokes
     * the provided render callback every frame.
     * @param renderCallback Function invoked per-frame with the active command buffer and swapchain.
     * @throws std::runtime_error If the AnvilApplication is uninitialized or `drawFrame` throws.
     * @attention Shader reloading happening here is not ideal.
     */
    void runAnvil(const std::function<void(VkCommandBuffer, AnvilSwapchain*)>& renderCallback);

    /**
     * @brief Shuts down the engine and safely destroys all Vulkan and window resources.
     *
     * Waits for the GPU device to idle before releasing handles.
     */
    void shutdownAnvil();

    /**
     * @brief Queues a callback function to be executed when a shader reload event occurs.
     *
     * Useful for hot-reloading shaders at runtime without restarting the application.
     * @param shaderCallback The function to execute when a reload is triggered.
     */
    void addShaderReloadCallback(const std::function<void()>& shaderCallback);

    /**
     * @brief Retrieves a reference to the active application window.
     * @return Reference to the AnvilWindow instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]] AnvilWindow& getAnvilWindow() const;

    /**
     * @brief Retrieves the core Vulkan context (instance, device, memory allocator).
     * @return Reference to the AnvilVulkanContext instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]] AnvilVulkanContext& getAnvilContext();

    /**
     * @brief Retrieves the active Vulkan swapchain.
     * @return Reference to the AnvilSwapchain instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]] AnvilSwapchain& getAnvilSwapchain();

    /**
     * @brief Retrieves the main renderer responsible for command buffer orchestration.
     * @return Reference to the AnvilRenderer instance.
     * @note The reference cannot be discarded.
     */
    [[nodiscard]] AnvilRenderer& getAnvilRenderer();
};

#endif //ANVIL_VK_APPLICATION_H

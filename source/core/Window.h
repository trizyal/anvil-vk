// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_WINDOW_H
#define ANVIL_VK_WINDOW_H

/**
 * @file Window.h
 * @brief GLFW Window creation, input polling, Vulkan surface creation, Framebuffer extent querying
 */

#include <string>

#include <volk.h>
#include <GLFW/glfw3.h>

/**
 * @brief Wrapper for GLFW window.
 *
 * Wraps a GLFW window and provides the functionality required by the
 * engine to interface with Vulkan. This includes creating a Vulkan
 * surface, querying the framebuffer size, polling window events, and
 * checking whether the window should close.
 *
 * @note This class is non-copyable and non-movable due to GLFW.
 */
class Window
{
public:
    /**
     * @brief Creates a new application window.
     * @param inWidth Initial window width in pixels.
     * @param inHeight Initial window height in pixels.
     * @param inTitle Window title bar text.
     * @todo Creates a resizeable window, should be configurable.
     */
    Window(uint32_t inWidth, uint32_t inHeight, std::string inTitle);

    /**
     * @brief Destroys the GLFW window and terminates GLFW.
     */
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

private:
    GLFWwindow* glfwWindow = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string anvilTitle;

public:
    /**
     * @brief Wrapper for glfwPollEvents.
     */
    static void pollEvents();

    /**
     * @brief Wrapper for glfwWindowShouldClose().
     * @return `true` if window should close, otherwise `false`
     */
    [[nodiscard]] bool bShouldClose() const;

    [[nodiscard]] bool isMinimised() const;

    /**
     * @return Window title.
     */
    [[nodiscard]] std::string getWindowTitle() const;

    /**
     * @brief Returns the underlying GLFW window handle.
     * @return Pointer to the native GLFW window.
     */
    [[nodiscard]] GLFWwindow* getGLFWWindow() const;

    /**
     * @brief Creates a Vulkan surface for this window.
     * @param inInstance Vulkan instance used to create the surface.
     * @return Vulkan surface.
     * @throws std::runtime_error If surface creation fails.
     */
    [[nodiscard]] VkSurfaceKHR createSurface(VkInstance inInstance) const;

    /**
     * @brief Wrapper for glfwGetFramebufferSize().
     * @return Framebuffer size in form of Vulkan extent.
     */
    [[nodiscard]] VkExtent2D getFramebufferExtent() const;
};

#endif //ANVIL_VK_WINDOW_H

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Window.h"

#include <iostream>
#include <stdexcept>
#include <utility>

#include "Ensure.h"
#include "Logger.h"
#include "Trace.h"

Window::Window(const uint32_t inWidth, const uint32_t inHeight, std::string inTitle)
    : width(inWidth), height(inHeight), anvilTitle(std::move(inTitle))
{
    SCOPE_CPU;

    LOG_TRACE("Creating Window");
    glfwInit();

    // No OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    glfwWindow = glfwCreateWindow(
        static_cast<int>(width),
        static_cast<int>(height),
        anvilTitle.c_str(),
        nullptr,
        nullptr);

    FATAL(glfwWindow, "Failed to create GLFW window");
    LOG_TRACE("Finishing creating AnvilWindow");
}

Window::~Window()
{
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

bool Window::bShouldClose() const
{
    SCOPE_CPU;
    return glfwWindowShouldClose(glfwWindow);
}

bool Window::isMinimised() const
{
    SCOPE_CPU;

    const VkExtent2D ext = getFramebufferExtent();
    if (ext.width == 0 || ext.height == 0)
    {
        return true;
    }

    return false;
}

void Window::pollEvents()
{
    SCOPE_CPU;

    glfwPollEvents();
}

std::string Window::getWindowTitle() const
{
    return anvilTitle;
}

GLFWwindow* Window::getGLFWWindow() const
{
    return glfwWindow;
}

VkSurfaceKHR Window::createSurface(VkInstance inInstance) const
{
    SCOPE_CPU
    LOG_TRACE("Creating Vulkan Surface.");
    VkSurfaceKHR surface;

    const VkResult res = glfwCreateWindowSurface(inInstance, glfwWindow, nullptr, &surface);
    FATAL(res == VK_SUCCESS, "Failed to create GLFW Vulkan Surface.");

    return surface;
}

VkExtent2D Window::getFramebufferExtent() const
{
    int fbWidth = 0;
    int fbHeight = 0;

    glfwGetFramebufferSize(glfwWindow, &fbWidth, &fbHeight);

    return VkExtent2D{
        .width = static_cast<uint32_t>(fbWidth),
        .height = static_cast<uint32_t>(fbHeight)
    };
}

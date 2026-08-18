// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Window.h"

#include <iostream>
#include <stdexcept>
#include <utility>

Window::Window(const uint32_t inWidth, const uint32_t inHeight, std::string inTitle)
    : width(inWidth), height(inHeight), anvilTitle(std::move(inTitle))
{
    std::cout << "Creating AnvilWindow..." << std::endl;
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

    if (!glfwWindow)
    {
        throw std::runtime_error("Failed to create GLFW window");
    }
    std::cout << "Finishing creating AnvilWindow" << std::endl;
}

Window::~Window()
{
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

bool Window::bShouldClose() const
{
    return glfwWindowShouldClose(glfwWindow);
}

void Window::pollEvents()
{
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
    VkSurfaceKHR surface;

    if (glfwCreateWindowSurface(inInstance, glfwWindow, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create GLFW surface");
    }

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

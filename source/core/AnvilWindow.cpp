// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilWindow.h"

#include <stdexcept>
#include <utility>

AnvilWindow::AnvilWindow(const uint32_t inWidth, const uint32_t inHeight, std::string inTitle)
    : width(inWidth), height(inHeight), anvilTitle(std::move(inTitle))
{
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
}

AnvilWindow::~AnvilWindow()
{
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

bool AnvilWindow::bShouldClose() const
{
    return glfwWindowShouldClose(glfwWindow);
}

void AnvilWindow::pollEvents()
{
    glfwPollEvents();
}

std::string AnvilWindow::getWindowTitle() const
{
    return anvilTitle;
}

GLFWwindow* AnvilWindow::getGLFWWindow() const
{
    return glfwWindow;
}

VkSurfaceKHR AnvilWindow::createSurface(VkInstance instance) const
{
    VkSurfaceKHR surface;

    if (glfwCreateWindowSurface(instance, glfwWindow, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create GLFW surface");
    }

    return surface;
}

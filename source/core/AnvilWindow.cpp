// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilWindow.h"

#include <iostream>
#include <stdexcept>
#include <utility>

AnvilWindow::AnvilWindow(const uint32_t inWidth, const uint32_t inHeight, std::string inTitle)
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

VkExtent2D AnvilWindow::getFramebufferExtent() const
{
    int fbWidth = 0;
    int fbHeight = 0;

    glfwGetFramebufferSize(glfwWindow, &fbWidth, &fbHeight);

    return VkExtent2D{
        static_cast<uint32_t>(fbWidth),
        static_cast<uint32_t>(fbHeight)
    };
}

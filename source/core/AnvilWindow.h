// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_WINDOW_H
#define ANVIL_VK_WINDOW_H

#include <cstdint>
#include <string>

#include <volk.h>
#include <GLFW/glfw3.h>

class AnvilWindow
{
private:
    GLFWwindow* glfwWindow = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string anvilTitle;

public:
    AnvilWindow(uint32_t inWidth, uint32_t inHeight, std::string inTitle);
    ~AnvilWindow();

    AnvilWindow(const AnvilWindow&) = delete;
    AnvilWindow& operator=(const AnvilWindow&) = delete;

    static void pollEvents();

    [[nodiscard]] bool bShouldClose() const;
    [[nodiscard]] std::string getWindowTitle() const;
    [[nodiscard]] GLFWwindow* getGLFWWindow() const;

    [[nodiscard]] VkSurfaceKHR createSurface(VkInstance instance) const;
    [[nodiscard]] VkExtent2D getFramebufferExtent() const;
};

#endif //ANVIL_VK_WINDOW_H

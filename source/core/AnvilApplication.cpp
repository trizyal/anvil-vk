// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilApplication.h"

#include <iostream>

#include <imgui.h>

#include "AnvilUILogger.h"

AnvilApplication::~AnvilApplication()
{
    shutdownAnvil();
}

void AnvilApplication::initializeAnvil(const AnvilApplicationCreateInfo& inCreateInfo)
{
    std::cout << "Initializing Anvil..." << std::endl;
    if (anvilInitialized)
    {
        return;
    }

    anvilWindow = std::make_unique<AnvilWindow>(inCreateInfo.width, inCreateInfo.height, inCreateInfo.title);
    anvilContext.initializeVulkanContext(*anvilWindow);
    anvilSwapchain.initializeSwapchain(anvilContext, anvilWindow->getFramebufferExtent());
    anvilRenderer.initializeRenderer(&anvilContext, &anvilSwapchain);
    anvilUIRenderer.initializeUIRenderer(&anvilContext, anvilWindow->getGLFWWindow(), &anvilSwapchain);

    anvilInitialized = true;
    std::cout << "Anvil initialization complete!" << std::endl;
}

void AnvilApplication::shutdownAnvil()
{
    if (!anvilInitialized)
    {
        return;
    }

    vkDeviceWaitIdle(anvilContext.anvilDevice);

    anvilUIRenderer.shutdownUIRenderer(&anvilContext);
    anvilRenderer.shutdownRenderer();
    anvilSwapchain.cleanup();
    anvilContext.destroyVulkanContext();

    anvilWindow.reset();

    anvilInitialized = false;
}

void AnvilApplication::runAnvil(const std::function<void(VkCommandBuffer, AnvilSwapchain*)>& renderCallback)
{
    if (!anvilInitialized)
    {
        throw std::runtime_error("AnvilApplication::runAnvil() called before initialization");
    }

    while (!anvilWindow->bShouldClose())
    {
        AnvilWindow::pollEvents();

        // Check for Shader Reload
        GLFWwindow* glwfWindow = anvilWindow->getGLFWWindow();
        bool isCtrl = glfwGetKey(glwfWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        bool isDot  = glfwGetKey(glwfWindow, GLFW_KEY_PERIOD) == GLFW_PRESS;
        bool isReloadPressed = isCtrl && isDot;

        if (isReloadPressed && !wasReloadPressed)
        {
            if (!shaderReloadQueue.empty())
            {
                std::cout << "[Anvil] Hot-reload triggered. Pausing GPU..." << std::endl;

                // Safely wait for all GPU work to finish BEFORE the project destroys pipelines
                vkDeviceWaitIdle(anvilContext.anvilDevice);

                for (auto& callback : shaderReloadQueue) {
                    callback();
                }

                std::cout << "[Anvil] Hot-reload complete." << std::endl;

                LOGUI("[Anvil] Shaders successfully reloaded!");
            }
        }
        wasReloadPressed = isReloadPressed;

        AnvilUIRenderer::BeginUIFrame();
        AnvilUILogger::DrawOverlay();

        anvilRenderer.drawFrame(*anvilWindow, renderCallback);

        AnvilUIRenderer::EndUIFrame();
    }

    vkDeviceWaitIdle(anvilContext.anvilDevice);
}

void AnvilApplication::addShaderReloadCallback(std::function<void()> callback)
{
    shaderReloadQueue.push_back(callback);
}

AnvilWindow& AnvilApplication::getAnvilWindow() const
{
    return *anvilWindow;
}

AnvilVulkanContext& AnvilApplication::getAnvilContext()
{
    return anvilContext;
}

AnvilSwapchain& AnvilApplication::getAnvilSwapchain()
{
    return anvilSwapchain;
}

AnvilRenderer& AnvilApplication::getAnvilRenderer()
{
    return anvilRenderer;
}

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilApplication.h"

#include <iostream>

#include "AnvilInput.h"
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

    AnvilInput::InitializeInputSystem(anvilWindow->getGLFWWindow());

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
        AnvilInput::UpdateInputs();

        // Check for Shader Reload
        bool isCtrl = AnvilInput::IsKeyPressed(GLFW_KEY_LEFT_CONTROL);
        bool isDot = AnvilInput::IsKeyPressed_Frame(GLFW_KEY_PERIOD);
        bool isReloadPressed = isCtrl && isDot;

        if (isReloadPressed)
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

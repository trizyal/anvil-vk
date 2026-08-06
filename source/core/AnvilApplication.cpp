// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilApplication.h"

#include <chrono>
#include <iostream>

#include "AnvilInput.h"
#include "UILogger.h"

void AnvilApplication::initializeAnvil(const AnvilApplicationCreateInfo& inCreateInfo)
{
    std::cout << "Initializing Anvil..." << std::endl;
    auto cpuStart = std::chrono::high_resolution_clock::now();
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
    auto cpuEnd = std::chrono::high_resolution_clock::now();
    auto initTime = std::chrono::duration<float, std::milli>(cpuEnd - cpuStart).count();
    std::cout << "Anvil initialization complete!" << std::endl;
    std::cout << "Initialization took:" << initTime << "ms" << std::endl;
}

void AnvilApplication::shutdownAnvil()
{
    if (!anvilInitialized)
    {
        return;
    }

    vkDeviceWaitIdle(anvilContext.anvilDevice);

    anvilWindow.reset();

    anvilInitialized = false;
}

void AnvilApplication::runAnvil(const std::function<void(VkCommandBuffer, VulkanSwapchain*)>& renderCallback)
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
        const bool isCtrl = AnvilInput::IsKeyPressed(GLFW_KEY_LEFT_CONTROL);
        const bool isDot = AnvilInput::IsKeyPressed_Frame(GLFW_KEY_PERIOD);
        const bool isReloadPressed = isCtrl && isDot;

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

        UIRenderer::BeginUIFrame();
        UILogger::DrawOverlay();

        anvilRenderer.drawFrame(*anvilWindow, renderCallback);

        UIRenderer::EndUIFrame();
    }

    vkDeviceWaitIdle(anvilContext.anvilDevice);
}

void AnvilApplication::addShaderReloadCallback(const std::function<void()>& shaderCallback)
{
    shaderReloadQueue.push_back(shaderCallback);
}

AnvilWindow& AnvilApplication::getAnvilWindow() const
{
    return *anvilWindow;
}

VulkanContext& AnvilApplication::getAnvilContext()
{
    return anvilContext;
}

VulkanSwapchain& AnvilApplication::getAnvilSwapchain()
{
    return anvilSwapchain;
}

AnvilRenderer& AnvilApplication::getAnvilRenderer()
{
    return anvilRenderer;
}

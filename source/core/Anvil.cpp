// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Anvil.h"

#include <chrono>
#include <iostream>

#include "Input.h"
#include "ScreenLogger.h"

void Anvil::initializeAnvil(const AnvilCreateInfo& inCreateInfo)
{
    std::cout << "Initializing Anvil..." << std::endl;
    const auto cpuStart = std::chrono::high_resolution_clock::now();
    if (initialized)
    {
        return;
    }

    window = std::make_unique<Window>(inCreateInfo.width, inCreateInfo.height, inCreateInfo.title);
    context.initializeVulkanContext(*window);
    swapchain.initializeSwapchain(context, window->getFramebufferExtent());
    renderer.initializeRenderer(&context, &swapchain);
    uiRenderer.initializeUIRenderer(&context, window->getGLFWWindow(), &swapchain);

    Input::InitializeInputSystem(window->getGLFWWindow());

    initialized = true;
    const auto cpuEnd = std::chrono::high_resolution_clock::now();
    const auto initTime = std::chrono::duration<float, std::milli>(cpuEnd - cpuStart).count();
    std::cout << "Anvil initialization complete!" << std::endl;
    std::cout << "Initialization took:" << initTime << "ms" << std::endl;
}

void Anvil::shutdownAnvil()
{
    if (!initialized)
    {
        return;
    }

    vkDeviceWaitIdle(context.device);

    window.reset();

    initialized = false;
}

void Anvil::runAnvil(const std::function<void(VkCommandBuffer, Swapchain*)>& renderCallback)
{
    if (!initialized)
    {
        throw std::runtime_error("AnvilApplication::runAnvil() called before initialization");
    }

    while (!window->bShouldClose())
    {
        auto frame_start = std::chrono::high_resolution_clock::now();

        Window::pollEvents();
        Input::UpdateInputs();

        // Check for Shader Reload
        const bool isCtrl = Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL);
        const bool isDot = Input::IsKeyPressed_Frame(GLFW_KEY_PERIOD);
        const bool isReloadPressed = isCtrl && isDot;

        if (isReloadPressed)
        {
            if (!shaderReloadQueue.empty())
            {
                std::cout << "[Anvil] Hot-reload triggered. Pausing GPU..." << std::endl;

                // Safely wait for all GPU work to finish BEFORE the project destroys pipelines
                vkDeviceWaitIdle(context.device);

                for (auto& callback : shaderReloadQueue) {
                    callback();
                }

                std::cout << "[Anvil] Hot-reload complete." << std::endl;

                LOGUI("[Anvil] Shaders successfully reloaded!");
            }
        }

        UIRenderer::BeginUIFrame();
        ScreenLogger::DrawOverlay();

        renderer.drawFrame(*window, renderCallback);

        UIRenderer::EndUIFrame();

        auto frame_end = std::chrono::high_resolution_clock::now();
        AnvilRenderer::engineStats.frameTime = std::chrono::duration<float, std::milli>(frame_end - frame_start).count();
    }

    vkDeviceWaitIdle(context.device);
}

void Anvil::addShaderReloadCallback(const std::function<void()>& shaderCallback)
{
    shaderReloadQueue.push_back(shaderCallback);
}

Window& Anvil::getWindow() const
{
    return *window;
}

VulkanContext& Anvil::getContext()
{
    return context;
}

Swapchain& Anvil::getSwapchain()
{
    return swapchain;
}

AnvilRenderer& Anvil::getRenderer()
{
    return renderer;
}

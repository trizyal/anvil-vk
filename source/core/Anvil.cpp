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

        // FIX: Catch the minimized window state
       if (window->isMinimised())
       {
           // Skip the rest of the loop entirely!
           // ImGui never starts, rendering never happens.
           continue;
       }

        // Check for Shader Reload
        const bool is_ctrl = Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL);
        const bool is_dot = Input::IsKeyPressed_Frame(GLFW_KEY_PERIOD);

        if (is_ctrl && is_dot)
        {
            triggerShaderHotReload();
        }

        UIRenderer::BeginUIFrame();
        ScreenLogger::DrawOverlay();

        // Render Error Dialog if hot reload failed
        if (bShaderErrorModalOpen)
        {
            // Call UI::DrawShaderErrorModal
        }

        renderer.drawFrame(*window, renderCallback);

        UIRenderer::EndUIFrame();

        auto frame_end = std::chrono::high_resolution_clock::now();
        AnvilRenderer::engineStats.frameTime = std::chrono::duration<float, std::milli>(frame_end - frame_start).count();
    }

    vkDeviceWaitIdle(context.device);
}

void Anvil::addShaderReloadCallback(const std::function<void()>& shaderCallback)
{
    // Wrap the legacy void callback so it fits the new internal queue.
    shaderReloadQueue.emplace_back([shaderCallback](std::string* /*outErr*/) -> bool {
        shaderCallback();
        return true;
    });
}

void Anvil::addShaderReloadCallback(const std::function<bool(std::string*)>& shaderCallback)
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

void Anvil::triggerShaderHotReload()
{
    if (shaderReloadQueue.empty()) return;

    std::cout << "[Anvil] Hot-reload triggered. Pausing GPU..." << std::endl;
    vkDeviceWaitIdle(context.device);

    bool all_succeeded = true;
    std::string accumulated_errors;

    for (auto& callback : shaderReloadQueue)
    {
        std::string err_log;
        if (!callback(&err_log)) // Pass the address of the string
        {
            all_succeeded = false;
            accumulated_errors += err_log + "\n";
        }
    }

    if (all_succeeded)
    {
        bShaderErrorModalOpen = false;
        activeShaderErrorLog.clear();
        std::cout << "[Anvil] Hot-reload complete." << std::endl;
        LOGUI("[Anvil] Shaders successfully reloaded!");
    }
    else
    {
        bShaderErrorModalOpen = true;
        activeShaderErrorLog = accumulated_errors;
        LOGUI("[Anvil] Shader hot-reload failed!", AnvilColor::Red);
    }
}

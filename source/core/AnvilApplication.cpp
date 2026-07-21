// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilApplication.h"

#include <iostream>

#include <imgui.h>

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
    anvilContext.initializeContext(*anvilWindow);
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

    anvilUIRenderer.destroy(&anvilContext);
    anvilRenderer.cleanup();
    anvilSwapchain.cleanup();
    anvilContext.cleanup();

    anvilWindow.reset();

    anvilInitialized = false;
}

void AnvilApplication::runAnvilRenderer(const std::function<void(VkCommandBuffer, AnvilSwapchain*)>& drawCallback)
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
                // TRIGGER THE ON-SCREEN NOTIFICATION (Show for 3 seconds)
                shaderReloadTimer = 3.0f;

                std::cout << "[Anvil] Hot-reload complete." << std::endl;
            }
        }
        wasReloadPressed = isReloadPressed;

        anvilUIRenderer.beginUIFrame();

        // Engine-Level UI Overlay (Shader Log)
        if (shaderReloadTimer > 0.0f)
        {
            // Decrease the timer by the time passed since last frame
            shaderReloadTimer -= ImGui::GetIO().DeltaTime;

            // Setup a borderless, non-interactive overlay window in the top-left
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

            // Get the position of the main application window
            ImGuiViewport* main_viewport = ImGui::GetMainViewport();

            // Offset 10 pixels from the top-left of the APP WINDOW, not the monitor
            ImVec2 overlayPos = ImVec2(main_viewport->WorkPos.x + 10.0f, main_viewport->WorkPos.y + 10.0f);

            ImGui::SetNextWindowPos(overlayPos, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.7f); // Slightly transparent dark background

            if (ImGui::Begin("Engine Overlay", nullptr, flags))
            {
                // Draw nice green text!
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[Anvil] Shaders successfully reloaded!");
            }
            ImGui::End();
        }


        anvilRenderer.drawFrame(*anvilWindow, drawCallback);

        anvilUIRenderer.endUIFrame();
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

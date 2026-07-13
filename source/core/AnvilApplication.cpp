// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilApplication.h"

#include <iostream>

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

    anvilRenderer.cleanup();
    anvilSwapchain.cleanup();
    anvilContext.cleanup();

    anvilWindow.reset();

    anvilInitialized = false;
}

void AnvilApplication::runAnvil()
{
    if (!anvilInitialized)
    {
        throw std::runtime_error("AnvilApplication::runAnvil() called before initialization");
    }

    while (!anvilWindow->bShouldClose())
    {
        AnvilWindow::pollEvents();

        // TODO: Test and fix drawFrame() function
        anvilRenderer.drawFrame(*anvilWindow);
    }

    vkDeviceWaitIdle(anvilContext.anvilDevice);
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

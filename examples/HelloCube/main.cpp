// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include <iostream>

#include "Anvil.h"

#include "HelloCube.h"

int main()
{
    Anvil anvil;
    anvil.initializeAnvil({
        .width = 1280,
        .height = 720,
        .title = "Anvil Hello Cube Example"
    });

    HelloCube project;
    project.initializeProject(anvil.getContext(), anvil.getSwapchain());

    // Register hot-reload event using the new boolean callback signature
    anvil.addShaderReloadCallback([&](std::string* /*err*/) -> bool {
        project.loadPipeline();
        return true;
    });

    try
    {
        // Use the RenderHooks struct to pass the callback
        RenderHooks hooks;
        hooks.onSwapchain = [&](VkCommandBuffer cmd, Swapchain* swapchain)
        {
            project.recordCommands(cmd, *swapchain);
        };

        anvil.runAnvil(hooks);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    project.cleanupProject();
    anvil.shutdownAnvil();

    return EXIT_SUCCESS;
}
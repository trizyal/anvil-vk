// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include <iostream>

#include "Anvil.h"

#include "TextureCube.h"

int main()
{
    try
    {
        Anvil anvil;
        anvil.initializeAnvil({
            .width = 1280,
            .height = 720,
            .title = "Anvil Texture Cube Example"
        });

        TextureCube project;
        project.initializeProject(anvil.getContext(), anvil.getSwapchain());

        // Register hot-reload event using the new boolean callback signature
        anvil.addShaderReloadCallback([&](std::string* /*err*/) -> bool {
            project.loadPipeline();
            return true;
        });
        // Use the RenderHooks struct to pass the callback
        RenderHooks hooks;
        hooks.onSwapchain = [&](VkCommandBuffer cmd, Swapchain* swapchain)
        {
            project.recordCommands(cmd, *swapchain);
        };

        anvil.runAnvil(hooks);

        project.cleanupProject();
        anvil.shutdownAnvil();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
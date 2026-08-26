// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include <iostream>

#include "Anvil.h"

#include "CesiumMan.h"

int main()
{
    try
    {
        Anvil anvil;
        anvil.initializeAnvil({
            .width = 1280,
            .height = 720,
            .title = "Anvil Rigging Example"
        });

        CesiumMan project;
        project.initializeProject(anvil.getContext(), anvil.getSwapchain());

        // Register hot-reload event
        anvil.addShaderReloadCallback([&]() {
            project.loadPipeline();
        });

        anvil.runAnvil([&](VkCommandBuffer cmd, Swapchain* swapchain)
        {
            project.recordCommands(cmd, *swapchain);
        });

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
